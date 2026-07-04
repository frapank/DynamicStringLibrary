# dstr vs sds - direct comparison

This document compares dstr against sds (Salvatore Sanfilippo's dynamic string library used by Redis). Benchmarked on 2026-07-04, dstr at commit `cf0c066` ("Merge pull request #9 from frapank/performance/inline-len-cap"). The sds version used for these benchmarks is commit 5347739... (2025-04-18).

## Executive summary

| Dimension                                                     | Winner                 | Margin                  | Why                                                                                                                                                           |
|---------------------------------------------------------------|------------------------|-------------------------|---------------------------------------------------------------------------------------------------------------------------------------------------------------|
| Concat/find/equal/case-conversion on large buffers            | tie                    | ~0-6%                   | both are memcpy/memcmp bound, glibc is the bottleneck, not the library                                                                                        |
| Single-character push                                         | dstr                   | ~2.6-3x                 | dstrpush_base is a dedicated path; sds falls back to the generic sdscatlen/sdsMakeRoomFor                                                                     |
| Duplication (dup)                                             | dstr                   | ~1.5x                   | dstrdup builds the header by hand; sdsdup goes through sdsnewlen which carries more generic branching (5 header types instead of 4, plus SDS_NOINIT handling) |
| Delimiter split, small scale (500k tokens, ~8MB)              | dstr                   | ~1.3x                   | vectorized memchr vs a scalar byte-by-byte scan                                                                                                               |
| Delimiter split, large scale (64MB, ~690k tokens)             | dstr                   | ~5-9x                   | same root cause, but the vectorized-memchr advantage grows with buffer size                                                                                   |
| Range/substring, in-place mutation                            | tie                    | ~0.99-1.00x             | dstrrange now mirrors sdsrange exactly: same clamping rules, same inclusive end, same memmove-in-place, zero allocations on both sides                        |
| Range/substring, independent copy (dup + range)               | dstr                   | ~1.5x                   | dup+range is the only way either API produces an independent copy now; dstrdup's hand-built header keeps its edge over sdsdup                                 |
| Range/substring, independent copy via a raw-bytes constructor | sds (structural)       | n/a                     | sds has sdsnewlen (build straight from a pointer+length, no dup needed); dstr has no equivalent constructor, so dup+range is its only option here             |
| Repeated len reads on one "hot" string                        | tie                    | ~1.00x (was sds ~1.65x) | dstrlen/dstrcap moved to static inline in dstr.h in this same branch, closing the call-overhead gap with sdslen/sdsalloc, see the note below                  |
| Header size for very short strings, under 32 bytes            | sds (theoretical)      | up to 2 bytes           | sds has a 1-byte fifth header tier dstr lacks, but the advantage disappears in practice almost every time because of glibc's allocator rounding               |
| Capacity growth strategy                                      | different philosophies | n/a                     | dstr keeps overhead steady around 15-37% and it's configurable; sds doubles up to 1MB (up to +64% overshoot) then switches to linear growth                   |

The rest of this doc has the full numbers and how I got them.

## How I measured this

CPU is an AMD Ryzen 5 7640U (Zen4, 12 logical threads), 30 GiB RAM, Void Linux, kernel 7.1.1_1, glibc 2.41. Both libraries were built with the same gcc 14.2.1 and the same flags, `-std=c11 -Wall -Wextra -O2`, no sanitizer in the timed runs, no LTO, and both use the plain glibc allocator.

Every number here is the median of 5 to 7 interleaved runs (dstr, sds, dstr, sds, and so on), and the full min/max spread for every single test sits in `bench/results_summary.csv` if you want to check how tight or loose a given number actually was.

## API mapping

| dstr                      | sds                           | notes                                                                                                                                    |
|---------------------------|-------------------------------|------------------------------------------------------------------------------------------------------------------------------------------|
| dstrnew                   | sdsnew / sdsnewlen            |                                                                                                                                          |
| dstrcat (2-arg)           | sdscat                        | both amortize growth by default                                                                                                          |
| dstrpush (2-arg)          | sdscatlen(s, &c, 1)           | sds has no dedicated char-push                                                                                                           |
| dstrdup                   | sdsdup                        |                                                                                                                                          |
| dstrfind                  | strstr                        | sds has no built-in find, it advertises itself as compatible with libc string functions so calling strstr directly is the idiomatic move |
| dstrsplit / dstrsplitfree | sdssplitlen / sdsfreesplitres |                                                                                                                                          |
| dstrrange                 | sdsrange                      | identical semantics                                                                                                                      |
| dstrtolower / dstrtoupper | sdstolower / sdstoupper       |                                                                                                                                          |
| dstrequal                 | sdscmp                        |                                                                                                                                          |
| dstrlen / dstrcap         | sdslen / sdsalloc             | both static inline in the header now, no more asymmetry here, see the note further down                                                  |

## Speed, micro-benchmarks

All times are ns/operation, median of 7 runs, unless noted otherwise.

| Operation                                                                       |          dstr |           sds | dstr/sds | Notes                                                                                                             |
|---------------------------------------------------------------------------------|--------------:|--------------:|---------:|-------------------------------------------------------------------------------------------------------------------|
| create_free (alloc+free, short string)                                          |       13.9 ns |       13.2 ns |     1.06 | tie                                                                                                               |
| concat_small (8B append, capacity already sufficient)                           |       12.9 ns |       13.3 ns |     0.97 | tie                                                                                                               |
| push_char (append 1 char)                                                       |        2.7 ns |        8.1 ns |     0.33 | dstr ~3.0x faster                                                                                                 |
| dup_large (dup a 1 MB string)                                                   |       17.6 us |       26.7 us |     0.66 | dstr ~1.5x faster                                                                                                 |
| find_found (needle in a 4 MB haystack)                                          |       33.5 us |       33.3 us |     1.01 | tie                                                                                                               |
| find_notfound (same, not found)                                                 |       33.5 us |       33.3 us |     1.01 | tie                                                                                                               |
| split (500k tokens, ~8 MB)                                                      |       22.8 ms |       30.5 ms |     0.75 | dstr ~1.3x faster, see the assembly breakdown below                                                               |
| range_copy_via_dup (dup a 2 MB source, then range down to 500B)                 |       37.1 us |       56.1 us |     0.66 | dstr ~1.5x faster, same dstrdup advantage as dup_large carries through                                            |
| range_inplace_only (range a pre-duped 200 KB string down to 500B, no dup timed) |        316 ns |        318 ns |     0.99 | tie, both are memmove-bound once the copy is out of the timed region                                              |
| range_copy (sds's sdsnewlen(s+start,len), no dup, no dstr equivalent)           |           n/a |       31.4 ns |      n/a | sds-only: skips duping the whole source, so it beats both of the above by orders of magnitude, see the note above |
| case_conversion (2 MB, alternating upper/lower)                                 |        794 us |        756 us |     1.05 | sds ~5% faster                                                                                                    |
| equal_same / equal_diff_last_byte (cmp 2 MB)                                    | ~33.6/33.5 us | ~33.5/33.2 us |     ~1.0 | tie                                                                                                               |

## Speed, larger scale

Micro-benchmarks only tell you so much, so I also ran two end-to-end tests closer to something you'd actually do with these libraries.

log_pipeline builds a synthetic log corpus (500,000 lines, ~62.5 MB, fixed-width so I can verify it exactly), splits it into lines, then scans and uppercases the ERROR-level lines (exactly 1 in 4, so 125,000 matches) into an aggregate report.

| Phase                               |   dstr |    sds |                dstr/sds |
|-------------------------------------|-------:|-------:|------------------------:|
| Build (500k concats)                |  90 ms |  87 ms |                    1.04 |
| Split (by line)                     |  45 ms | 224 ms | 0.20, dstr ~5.0x faster |
| Process (find + uppercase + concat) |  19 ms |  20 ms |                    0.96 |
| Total pipeline                      | 153 ms | 331 ms | 0.46, dstr ~2.2x faster |

big_buffer is a single 64 MB buffer with one find/split/toupper pass over the whole thing, one big operation rather than lots of small repeated ones.

| Operation                                       |    dstr |      sds |                dstr/sds |
|-------------------------------------------------|--------:|---------:|------------------------:|
| find (marker near the end)                      |  3.2 ms |   3.2 ms |                    1.00 |
| split (delimiter every 97 bytes, 691,844 parts) | 20.8 ms | 187.1 ms | 0.11, dstr ~9.0x faster |
| toupper (whole buffer)                          | 26.6 ms |  25.5 ms |                    1.04 |

Notice the split gap actually grows with scale: 1.3x at 500k tokens/8MB, 5.0x at 500k lines/62MB, 9.0x at 690k tokens/64MB. That's not noise, it holds across 5 interleaved runs with under 10% spread almost everywhere (check `bench/results_summary.csv` if you want the raw numbers). And I didn't just infer the cause from timings, I went and checked the assembly.

### Why split diverges so much: memchr vs a scalar scan

> [!NOTE]
> dstrfind is compared against strstr (glibc's heavily optimized Two-Way algorithm) because that's the idiomatic usage sds itself recommends, not against some naive implementation I wrote just for this comparison.

I disassembled both benchmark binaries with objdump, same gcc -O2 build used everywhere else. dstrsplit, through its internal _dstr_find_from helper, hands the delimiter search off to glibc's memchr:

```asm
; dstr.o -- dstrsplit's outer loop, one memchr call per token found
3f5f: mov    %rbp,%rdi
3f62: call   memchr@plt      ; SIMD (AVX2 on this CPU), tens of bytes/instruction
3f6a: test   %rax,%rax
3f6d: jne    3f30 <dstrsplit+0xc0>
```

sdssplitlen instead compares one byte at a time with a scalar cmp, in a hand-written loop (this is the seplen==1 fast path in sds.c):

```asm
; sds.o -- sdssplitlen's inner loop, one cmp per byte of the buffer
5198: mov    0x10(%rsp),%rax
519d: movzbl (%rax),%eax     ; load the separator byte once
51a0: cmp    %al,(%rdi)      ; compare 1 byte at a time
51a2: jne    50fa <sdssplitlen+0xca>
```

On a small buffer this barely registers, the CPU chews through a few million scalar iterations in a few milliseconds either way. On a buffer tens of megabytes wide, though, the scalar scan has to touch every single byte one instruction at a time, while memchr processes 16 to 32+ bytes per instruction using the CPU's vector units. The gap grows roughly linearly with buffer size, which is exactly the pattern I measured: 1.3x, then 5.0x, then 9.0x as the scale goes up.

## Memory

### Header overhead, raw bytes, no allocator rounding

I got these by reading the type byte at s[-1] (a documented public contract in both libraries) through each library's own public API.

|  string length | dstr header | sds header | note                                                                                                                                                                         |
|---------------:|------------:|-----------:|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
|              0 |         3 B |        3 B | sds forces header type 8 even for an empty string, it says so right in the source: type 5 can't track later appends well                                                     |
|      5, 20, 31 |         3 B |        1 B | sds has a fifth header tier (sdshdr5) that dstr just doesn't have, for strings under 32 bytes                                                                                |
|        32, 100 |         3 B |        3 B |                                                                                                                                                                              |
|            255 |         5 B |        3 B | dstr already bumped to 5 B here because its threshold is on requested capacity (len+1=256, which is over 255); sds's threshold is on logical length (255 is still under 256) |
|      256, 1000 |         5 B |        5 B |                                                                                                                                                                              |
|          65535 |         9 B |        5 B | dstr needs cap=65536 (len+1), which crosses UINT16_MAX and bumps the header type one byte earlier than it strictly has to                                                    |
| 65536, 1000000 |         9 B |        9 B |                                                                                                                                                                              |

### Bytes actually allocated, malloc_usable_size, glibc rounding included

|        length |    dstr |     sds |
|--------------:|--------:|--------:|
|          0-20 |    24 B |    24 B |
|         31-32 |    40 B |    40 B |
|           100 |   104 B |   104 B |
|       255-256 |   264 B |   264 B |
|          1000 |  1016 B |  1016 B |
|         65535 | 65560 B | 65544 B |
| 65536-1000000 |   equal |   equal |

sds's theoretical 1-byte-header advantage basically disappears in practice: glibc rounds small allocations up to a minimum of 24 usable bytes no matter what, so a 1-2 byte header difference doesn't turn into real savings under roughly 32 bytes. The advantage only becomes real at the 65535 boundary, 5 real bytes saved, for the structural reason explained above.

### Growth strategy, capacity after incremental appends

| length reached |    dstr cap | dstr overhead |     sds cap | sds overhead |
|---------------:|------------:|--------------:|------------:|-------------:|
|            100 |       121 B |          +21% |       112 B |         +12% |
|          1,000 |     1,369 B |          +37% |     1,008 B |        +0.8% |
|         10,000 |    10,390 B |         +3.9% |    16,368 B |         +64% |
|        100,000 |   118,342 B |          +18% |   131,056 B |         +31% |
|      1,000,000 | 1,347,984 B |          +35% | 1,048,560 B |        +4.9% |

Two different philosophies here, and I wouldn't call either one strictly better. dstr grows only when it actually needs to (after the _dstr_grow_cap fix made in this same branch, see the note below), adding 50% of the current capacity with a 16 byte floor, and that's configurable via dstr_options.h. Overhead stays fairly flat, roughly 15-37%, no matter the scale. sds doubles capacity until it crosses 1 MB (SDS_MAX_PREALLOC), then switches to linear growth, +1 MB per step. That gives it a sawtooth overhead pattern: very low right after a doubling, very high right before the next one (+64% is the worst I measured), but it actually ends up leaner than dstr once you're past the 1 MB mark, thanks to that linear growth kicking in.

### Many small strings alive at once, 200,000 strings, 64 bytes each

| metric                          |         dstr |          sds |
|---------------------------------|-------------:|-------------:|
| Process RSS delta               | 16,646,144 B | 17,010,688 B |
| Bytes in use in the glibc arena | 16,009,456 B | 16,009,520 B |

At 64 bytes per string both libraries land on the same header size, 3 bytes, so it's no surprise arena usage is nearly identical here too, under 0.001% apart.
