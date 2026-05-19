# dstr - Dynamic String Library for C

## Overview

dstr is a header-only dynamic string library for C (C11+). It exposes a `char*` interface backed by a hidden allocation header that stores length and capacity. Strings are heap-allocated and may be reallocated automatically during operations.

Requires gcc or clang. Supported architectures: x86, x86-64, ARM, ARM64.

## Build

Define the implementation once in a single translation unit:

```c
#define DSTR_IMPLEMENTATION
#include "dstr.h"
```

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
dstr s = dstrnew("hello", 64); // 64 capacity
```

If a capacity is provided and is smaller than the string length, it is automatically increased to fit. The allocation size matches the string length plus null terminator when no capacity is given.

### Length and capacity

```c
size_t len = dstrlen(s);
size_t cap = dstrcap(s);
```

Both return `size_t`.

### Concatenate (`dstr` + `char*`)

```c
s = dstrcat(s, "world");
s = dstrcat(s, "world", 128); // 128 capacity
```

The first argument must be a `dstr`. The second is a `const char*`. Use `dstrappend` when the second argument is also a `dstr`.

### Append (`dstr` + `dstr`)

```c
s = dstrappend(s, s2);
s = dstrappend(s, s2, 128); // 128 capacity
```

Both arguments must be `dstr`. Passing a `char*` as the second argument is not supported.

### Reserve capacity

```c
s = dstrreserve(s, 256);
```

Grows the allocation to at least `new_cap`. No-op if current capacity is already sufficient. The returned pointer must be reassigned as the buffer may move during reallocation. If the header type changes due to the new size, a new allocation is made and the old one is freed.

### Duplicate

```c
dstr copy = dstrdup(s);
```

### Clear

```c
dstrclear(s);
```

Resets the string content without freeing the allocation.

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

### Free

```c
dstrfree(s);
```

## Automatic cleanup

`dstrauto` declares a `dstr` with `__attribute__((cleanup))`.  
the string is freed automatically when it goes out of scope:

```c
dstrauto dstr s = dstrnew("hello");
```

## Shortcut macro

When `DSTR_SHORTCUT` is defined in `dstr_option.h', `$()` works as an alias for `dstrnew`:

```c
#define DSTR_SHORTCUT
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
- **dstrfind** — find first or last occurrence of a char or substring
- **dstrpush** — append a single byte with amortized growth
- **dstrtrim** — strip leading/trailing whitespace or a given charset
- **dstrrange** — return a substring by start/end index
- **dstrsplit** — split by delimiter into an array of `dstr`
- **dstrtolower / dstrtoupper** — in-place ASCII case conversion

## License

BSD-2-Clause
