# dstr - Dynamic String Library for C

## Overview

dstr is a dynamic string library for C (C11+). It exposes a `char*` interface backed by a hidden allocation header that stores length and capacity. Strings are heap-allocated and may be reallocated automatically during operations.

Requires gcc or clang. Supported architectures: x86, x86-64, ARM, ARM64.

## Build

The library ships as two files: `dstr.h` and `dstr.c`. Compile and link using the provided Makefile:

```sh
# Build both static and shared libraries
make

# Build static library only
make libstatic

# Build shared library only
make libshared
```

This produces `libdstr.a` and `libdstr.so`. Link against whichever suits your project:

```sh
cc -std=c11 your_file.c libdstr.a -o your_program
```

## Configuration

`dstr_options.h` is included by `dstr.c` and is the single place to configure the library. Create it in the same directory as `dstr.c` before building. Available options:

**Custom allocator** — override the default `malloc`, `realloc`, and `free`:

```c
// dstr_options.h
#define DSTR_MALLOC(sz)        my_malloc(sz)
#define DSTR_REALLOC(ptr, sz)  my_realloc(ptr, sz)
#define DSTR_FREE(ptr)         my_free(ptr)
```

If not defined, these fall back to the standard allocators.

**Growth strategy** - controls how `dstrcat`, `dstrappend`, and `dstrpush` (the two-argument, non-custom forms) grow the buffer when the current capacity is not enough:

```c
// dstr_options.h
#define DSTR_GROWTH_ENABLED 1   /* 1 = amortized growth, 0 = allocate exactly what is needed */
#define DSTR_GROWTH_PERCENT 50  /* extra capacity added on growth, as a percent of the current capacity */
```

With `DSTR_GROWTH_ENABLED` set to 1 (the default), a growth only happens when the requested size exceeds the current capacity, and it adds `DSTR_GROWTH_PERCENT` percent of headroom on top (50 means the new capacity is roughly 1.5x the old one), so repeated small appends do not reallocate on every call. Setting it to 0 disables the headroom: every growth allocates exactly what is needed, trading more frequent reallocations for a tighter memory footprint. Either way, capacity is left untouched whenever it is already sufficient. The `_custom` variants of `dstrcat`, `dstrappend`, and `dstrpush`, as well as the `cap` argument of `dstrnew`, always honor the caller-supplied capacity and are not affected by these options.

## Types

`dstr` is a `char*`. The internal headers (`dstrhd8`, `dstrhd16`, `dstrhd32`, `dstrhd64`) are implementation details and not part of the public API. The header type used for a given string is selected automatically based on the required capacity.

## Memory layout

```
[dstrhd{8|16|32|64}][char buffer]
```

The `dstr` pointer points to the buffer. The header immediately precedes it in memory and stores `len`, `cap`, and a `type` byte used to identify the header variant at runtime. The `type` byte is always at `s[-1]`.

Header sizes grow with string capacity:

| Type      | Max capacity       |
|-----------|--------------------|
| dstrhd8   | 255                |
| dstrhd16  | 65,535             |
| dstrhd32  | 4,294,967,295      |
| dstrhd64  | 2^64 - 1           |

## API

### Create

```c
dstr s = dstrnew("hello");
dstr s = dstrnew("hello", 64); // pre-allocate 64 bytes of capacity
```

If a capacity is provided and is smaller than the string length, it is automatically increased to fit. When no capacity is given, the allocation matches the string length plus null terminator.

### Length and capacity

```c
size_t len = dstrlen(s);
size_t cap = dstrcap(s);
```

Both return `size_t`.

### Concatenate (`dstr` + `char*`)

```c
s = dstrcat(s, "world");
s = dstrcat(s, "world", 128); // hint a capacity of 128 bytes
```

The first argument must be a `dstr`. The second is a `const char*`. Use `dstrappend` when the second argument is also a `dstr`.

### Append (`dstr` + `dstr`)

```c
s = dstrappend(s, s2);
s = dstrappend(s, s2, 128); // hint a capacity of 128 bytes
```

Both arguments must be `dstr`. Passing a `char*` as the second argument is not supported.

### Reserve capacity

```c
s = dstrreserve(s, 256);
```

Grows the allocation to at least `new_cap`. No-op if the current capacity is already sufficient. The returned pointer must be reassigned as the buffer may move during reallocation. If the header type changes due to the new size, a new allocation is made and the old one is freed.

### Duplicate

```c
dstr copy = dstrdup(s);
```

### Clear

```c
dstrclear(s);
```

Resets `len` to zero and writes a null terminator without freeing the allocation. Previous content beyond the null terminator remains in memory.

### Zero

```c
dstrzero(s);
```

Resets `len` to zero and zeroes the entire buffer with `memset`. Use this when the previous content must not remain readable in memory.

### Compare

```c
if (dstrequal(s1, s2)) { ... }
```

Returns `true` if the strings are equal, `false` otherwise. Both arguments must be `dstr`. For `dstr` vs `char*` comparisons use `strcmp` directly.

### Case conversion

```c
dstrtolower(s);
dstrtoupper(s);
```

Converts ASCII letters in place. Bytes outside `'A'-'Z'`/`'a'-'z'`, including non-ASCII bytes, are left untouched. Length and capacity are unchanged; no reallocation occurs. No-op if `s` is `NULL`.

### Find substring

```c
ssize_t index = dstrfind(s, "needle");

```

Searches for the first occurrence of a null-terminated C string (`needle`) inside the dynamic string `s`. Returns the zero-based index of the first match, or `-1` if the substring is not found. It safely returns `-1` if either argument is `NULL`, if the needle is an empty string, or if the needle is longer than the string itself.

### Range (substring)

```c
dstr sub = dstrrange(s, 0, 3);   // first 3 characters
dstr sub = dstrrange(s, -3, -1); // second-to-last and third-to-last characters
dstr sub = dstrrange(s, 2, -1);  // from index 2 up to (not including) the last character
```

Returns a new `dstr` holding the `[start, end)` slice of `s` (`end` is exclusive). Negative indices count from the end of the string, i.e. `-1` is the last character. Out-of-range indices are clamped instead of erroring, and a `start` at or past `end` yields an empty `dstr`. Returns `NULL` only if `s` is `NULL` or allocation fails. Free the result with `dstrfree`.

### Split

```c
size_t count;
dstr* parts = dstrsplit(s, ",", &count);
for (size_t i = 0; i < count; i++)
    puts(parts[i]);
dstrsplitfree(parts, count);
```

Splits `s` on every occurrence of `delim`, returning a newly allocated array of `dstr`. Adjacent or leading/trailing delimiters produce empty (`""`) elements. Returns `NULL` if `s`/`delim` is `NULL`, `delim` is empty, or allocation fails. The array and every element must be released together with `dstrsplitfree`.

### Free

```c
dstrfree(s);
```

## Automatic cleanup

`dstrauto` declares a `dstr` with `__attribute__((cleanup))`.  
The string is freed automatically when it goes out of scope:

```c
dstrauto dstr s = dstrnew("hello");
```

## Shortcut macro

`$()` can be enabled by changing `DSTR_SHORTCUT_ENABLE` in `dstr.h`. It works as an alias for `dstrnew`:

```c
// dstr_options.h
#define DSTR_SHORTCUT
```

```c
#include "dstr.h"

dstr s = $("hello");
dstr s = $("hello", 64);
```

## Notes

- Strings are always null-terminated.
- All functions that return `dstr` may return a reallocated pointer; always reassign.
- Capacity arguments are hints and are silently increased if insufficient.

## Roadmap

- **dstrinsert** — insert substring at a given index
- **dstrtrim** — strip leading/trailing whitespace or a given charset

## License

BSD-2-Clause
