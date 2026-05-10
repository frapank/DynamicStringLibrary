# dstr - Dynamic String Library for C

## Overview

dstr is a header-only library for dynamic strings in C (C11+). It provides a `char*` interface backed by an internal allocation header that stores length and capacity information.

Strings are heap allocated and may be reallocated automatically during operations such as concatenation. The library is designed to integrate with standard C code without requiring wrappers or custom string types in user code.

## Build

Define the implementation in a single source file:

```c
#define DSTR_IMPLEMENTATION
#include "dstr.h"
```

## Types

* `dstr`: public string type (`char*`)
* `dstrhd`: internal header containing metadata and buffer
* `dstrhdp`: pointer to `dstrhd`

## Allocation model

String allocation is handled through `dstrnew`.

* If only a `const char*` is provided, the library allocates at least 10 bytes or more if the string is longer.
* If a second argument is provided, it is used as the initial capacity.
* If the provided capacity is smaller than the required string length, it is automatically increased to fit the string.

This rule applies to all allocation-based functions that accept optional capacity arguments: the value is treated as a hint and never allowed to produce an invalid allocation.

## Generic function dispatch

Some functions use C11 `_Generic` to provide type-aware dispatch at compile time.

Functions such as `dstrlen`, `dstrdup`, and `dstrclear` automatically resolve to different implementations depending on the input type:

* `dstr` (raw string pointer)
* `dstrhd` (header)
* `dstrhdp` (pointer to header)

This allows the same function name to correctly handle different representations of the same string data without requiring manual conversion or explicit function selection.

## Usage

### Create a string

You can create a string with the following code:

```c
dstr s = dstrnew("hello");
```

The default capacity of the string is 10. If the string is longer, the capacity will automatically match the string length.
You can assign a custom capacity with:

```c
dstr s = dstrnew("hello", 64);
```

If the specified capacity is smaller than the required size, it is automatically adjusted.

### Get length

`dstrlen` returns an `unsigned int` with the string length. It works with `dstr`, `dstrhd`, and `dstrhdp`:

```c
unsigned int len = dstrlen(s);
```

### Concatenate

`dstrcat` can concatenate two strings. The first argument should be a `dstr`, and the second can be a simple `char*`:

```c
s = dstrcat(s, " world");
```

`dstrcat` as with `dstrnew`, you can also specify a capacity:

```c
s = dstrcat(s, " world", 128);
```

If the provided capacity is insufficient, it is increased automatically.

### Duplicate

`dstrdup` returns a duplicate of the provided string. It works with `dstr`, `dstrhd`, and `dstrhdp`:

```c
dstr copy = dstrdup(s);
```

### Clear string

`dstrclear` clears the string while retaining the allocated memory. It works with `dstr` and `dstrhdp`:

```c
dstrclear(s);
```

### Resize string

`dstrresize` adjusts the capacity of an existing string. It accepts both `dstr` and `dstrhdp` types, returning a potentially reallocated pointer:

```c
s = dstrresize(s, 128);
hd = dstrresize(hd, 256);
```

If the provided capacity is insufficient, it is increased automatically.
The returned pointer **must be reassigned**, as the buffer may have moved during reallocation.

### Compare strings

`dstrcmp` compares two dynamic strings and returns `1` if they are equal, `0` otherwise. The function accepts both `dstr` and `dstrhd*` types in any order:

```c
if(dstrcmp(s, hd)) {
    // ...
}
```

This function **does not** accept raw `char*` arguments. If you need to compare a `dstr` with a `char*`, use the standard C `strcmp` function instead.

### Free string

`dstrfree` frees the string:

```c
dstrfree(s);
```

## Memory layout

Each string is stored as:

```
[dstrhd][char buffer]
```

The `dstr` pointer refers directly to the buffer, while metadata (length and capacity) is stored in the hidden header.

## Notes

* Strings are always null-terminated.
* Functions may return reallocated pointers and must be reassigned.
* Default minimum allocation size is 10 bytes.
* Capacity arguments are hints and never override minimum required size.
* Memory ownership is handled entirely by the library.

## TODO

* dstrinsert: insert substring at position
* dstrfind: search character
* dstrpush: append single character

## License

BSD-2-Clause
