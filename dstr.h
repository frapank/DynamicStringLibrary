/*
 * Copyright (c) 2026 Francesco Carbone (@frapank)
 * SPDX-License-Identifier: BSD-2-Clause
 */

#ifndef DSTR_H
#define DSTR_H

#if !defined(__GNUC__) && !defined(__clang__)
#error "Unsupported compiler, DynamicStringLibrary supports only gcc and clang"
#endif
#if !defined(__STDC_VERSION__) || __STDC_VERSION__ < 201112L
#error "DynamicStringLibrary supports only C11+ versions"
#endif
#if !defined(__x86_64__) && !defined(_M_X64) && !defined(__i386__) &&          \
    !defined(_M_IX86) && !defined(__aarch64__) && !defined(__arm__)
#error                                                                         \
    "DynamicStringLibrary supports only common architectures: x86, x86-64, ARM, ARM64"
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef _WIN32
#include <stddef.h>
typedef ptrdiff_t ssize_t;
#else
#include <sys/types.h>
#endif

#define DSTR_SHORTCUT_ENABLED 0 /* 1 enabled | 0 disabled */

#define GET_DSTRNEW(_1, _2, NAME, ...) NAME
#define dstrnew(...)                                                           \
    GET_DSTRNEW(__VA_ARGS__, dstrnew_custom, dstrnew_base)(__VA_ARGS__)

#define GET_DSTRPUSH(_1, _2, _3, NAME, ...) NAME
#define dstrpush(...)                                                          \
    GET_DSTRPUSH(__VA_ARGS__, dstrpush_custom, dstrpush_base)(__VA_ARGS__)

#define GET_DSTRCAT(_1, _2, _3, NAME, ...) NAME
#define dstrcat(...)                                                           \
    GET_DSTRCAT(__VA_ARGS__, dstrcat_custom, dstrcat_base)(__VA_ARGS__)

#define GET_DSTRAPPEND(_1, _2, _3, NAME, ...) NAME
#define dstrappend(...)                                                        \
    GET_DSTRAPPEND(__VA_ARGS__, dstrappend_custom, dstrappend_base)(__VA_ARGS__)

#define W_UNUSED_RESULT __attribute__((warn_unused_result))

typedef char* dstr;

enum dstrhd_type {
    DSTRHD_TYPE_8 = 0,
    DSTRHD_TYPE_16 = 1,
    DSTRHD_TYPE_32 = 2,
    DSTRHD_TYPE_64 = 3
};

struct __attribute__((packed)) dstrhd8 {
    uint8_t len;
    uint8_t cap;
    uint8_t type;
    char buf[];
};

struct __attribute__((packed)) dstrhd16 {
    uint16_t len;
    uint16_t cap;
    uint8_t type;
    char buf[];
};

struct __attribute__((packed)) dstrhd32 {
    uint32_t len;
    uint32_t cap;
    uint8_t type;
    char buf[];
};

struct __attribute__((packed)) dstrhd64 {
    uint64_t len;
    uint64_t cap;
    uint8_t type;
    char buf[];
};

#define DSTRGETHDR(n, s)                                                       \
    ((struct dstrhd##n*)((char*)(s) - sizeof(struct dstrhd##n)))
#define DSTRGETTYPE(s) ((uint8_t)((s)[-1]))

/*
 * dstrnew(msg)
 * dstrnew(msg, cap)
 *
 * Create a new dstr from the null-terminated string msg.
 * The two-argument form pre-allocates at least cap bytes; useful when the
 * string will grow soon after creation.
 * Returns NULL on allocation failure.
 */
dstr dstrnew_base(const char* msg) W_UNUSED_RESULT;
dstr dstrnew_custom(const char* msg, size_t cap) W_UNUSED_RESULT;

/*
 * dstrdup(s)
 *
 * Return a deep copy of s with the same length and capacity.
 * Returns NULL if s is NULL or allocation fails.
 */
dstr dstrdup(dstr s) W_UNUSED_RESULT;

/*
 * dstrfree(s)
 *
 * Release the memory owned by s.  Passing NULL is a no-op.
 */
void dstrfree(dstr s);

/*
 * dstrfind(s, needle)
 *
 * Search for the first occurrence of the substring needle inside s.
 * Returns the index of the first match, or -1 if not found or if
 * s/needle are NULL or needle is empty.
 */
ssize_t dstrfind(dstr s, const char* needle) W_UNUSED_RESULT;

/*
 * dstrrange(s, start, end)
 *
 * Keep only the [start, end] slice of s, in place (end is inclusive).
 * Negative indices count from the end of the string, as in s[len + start].
 * Out-of-range indices are clamped, and a start past end or past the end
 * of the string yields an empty string. No allocation, no new pointer:
 * the existing buffer is shifted with memmove and truncated in place.
 * No-op if s is NULL or empty.
 */
void dstrrange(dstr s, ssize_t start, ssize_t end);

/*
 * dstrsplit(s, delim, out_count)
 *
 * Split s on every occurrence of the null-terminated delim, returning a
 * newly allocated array of dstr. Adjacent or leading/trailing delimiters
 * produce empty ("") elements. If out_count is not NULL, the number of
 * elements is written to it.
 * Returns NULL (and *out_count = 0) if s or delim is NULL, delim is
 * empty, or on allocation failure. Free the result with dstrsplitfree.
 */
dstr* dstrsplit(dstr s, const char* delim, size_t* out_count) W_UNUSED_RESULT;

/*
 * dstrsplitfree(parts, count)
 *
 * Free an array returned by dstrsplit, including each element. Passing
 * NULL is a no-op.
 */
void dstrsplitfree(dstr* parts, size_t count);

/*
 * dstrlen(s)  - number of characters in s, excluding the null terminator.
 * dstrcap(s)  - total allocated capacity in bytes.
 */
static inline size_t dstrlen(dstr s) W_UNUSED_RESULT;
static inline size_t dstrlen(dstr s)
{
    if (!s)
        return 0;
    switch ((enum dstrhd_type)DSTRGETTYPE(s)) {
        case DSTRHD_TYPE_8:
            return DSTRGETHDR(8, s)->len;
        case DSTRHD_TYPE_16:
            return DSTRGETHDR(16, s)->len;
        case DSTRHD_TYPE_32:
            return DSTRGETHDR(32, s)->len;
        case DSTRHD_TYPE_64:
            return DSTRGETHDR(64, s)->len;
    }
    return 0;
}

static inline size_t dstrcap(dstr s) W_UNUSED_RESULT;
static inline size_t dstrcap(dstr s)
{
    if (!s)
        return 0;
    switch ((enum dstrhd_type)DSTRGETTYPE(s)) {
        case DSTRHD_TYPE_8:
            return DSTRGETHDR(8, s)->cap;
        case DSTRHD_TYPE_16:
            return DSTRGETHDR(16, s)->cap;
        case DSTRHD_TYPE_32:
            return DSTRGETHDR(32, s)->cap;
        case DSTRHD_TYPE_64:
            return DSTRGETHDR(64, s)->cap;
    }
    return 0;
}

/*
 * dstrclear(s)
 *
 * Set the length to 0 and write a null terminator at s[0].
 * The underlying buffer is kept; capacity is unchanged.
 */
void dstrclear(dstr s);

/*
 * dstrzero(s)
 *
 * Like dstrclear, but also zeroes out the entire buffer.
 */
void dstrzero(dstr s);

/*
 * dstrequal(s1, s2)
 *
 * Return true if s1 and s2 have the same length and contents.
 * Returns false if either pointer is NULL.
 */
bool dstrequal(dstr s1, dstr s2) W_UNUSED_RESULT;

/*
 * dstrtolower(s)
 *
 * Convert every ASCII uppercase letter ('A'-'Z') in s to lowercase, in
 * place. Bytes outside that range, including non-ASCII bytes, are left
 * untouched. Length and capacity are unchanged. No-op if s is NULL.
 */
void dstrtolower(dstr s);

/*
 * dstrtoupper(s)
 *
 * Convert every ASCII lowercase letter ('a'-'z') in s to uppercase, in
 * place. Bytes outside that range, including non-ASCII bytes, are left
 * untouched. Length and capacity are unchanged. No-op if s is NULL.
 */
void dstrtoupper(dstr s);

/*
 * dstrreserve(s, new_cap)
 *
 * Ensure s has at least new_cap bytes of capacity.  No-op if the current
 * capacity is already sufficient.  May reallocate; always use the returned
 * pointer.  Returns NULL on failure, leaving the original allocation intact.
 */
dstr dstrreserve(dstr s, size_t new_cap) W_UNUSED_RESULT;

/*
 * dstrpush(s1, c)
 * dstrpush(s1, c, cap)
 *
 * Append a single character to the end of a dynamic dstr. No-op if the current
 * capacity is already sufficient.  May reallocate; always use the returned
 * pointer.  Returns NULL on failure, leaving the original allocation intact.
 *
 */
dstr dstrpush_base(dstr s1, const char c) W_UNUSED_RESULT;
dstr dstrpush_custom(dstr s1, const char c, size_t cap) W_UNUSED_RESULT;

/*
 * dstrcat(s1, s2)
 * dstrcat(s1, s2, cap)
 *
 * Append the null-terminated C string s2 to s1.  The three-argument form
 * hints at a desired capacity after the operation, which can reduce
 * reallocations when multiple appends are planned.
 * May reallocate s1; always use the returned pointer.
 * Returns NULL on failure.
 */
dstr dstrcat_base(dstr s1, const char* s2) W_UNUSED_RESULT;
dstr dstrcat_custom(dstr s1, const char* s2, size_t cap) W_UNUSED_RESULT;

/*
 * dstrappend(s1, s2)
 * dstrappend(s1, s2, cap)
 *
 * Same as dstrcat, but s2 is a dstr rather than a plain C string.
 */
dstr dstrappend_base(dstr s1, const dstr s2) W_UNUSED_RESULT;
dstr dstrappend_custom(dstr s1, const dstr s2, size_t cap) W_UNUSED_RESULT;

/*
 * dstrinsert(s1, idx, s2)
 *
 * Insert the null-terminated string s2 into s1 at idx, in place. Negative
 * indices count from the end of the string, as in s[len + idx]. Out-of-range
 * indices are clamped instead of erroring, so an idx at or before the start
 * prepends and an idx at or past the end appends.
 * May reallocate s1; always use the returned pointer.
 * Returns s1 unchanged if s1 or s2 is NULL, or if s2 is empty.
 * Returns NULL on allocation failure.
 */
dstr dstrinsert(dstr s1, ssize_t idx, const char* s2) W_UNUSED_RESULT;

/*
 * dstrauto
 *
 * Storage-class attribute that calls dstrfree automatically when the variable
 * goes out of scope (GCC/Clang cleanup extension).
 *
 *   dstrauto s = dstrnew("hello");
 */
#define dstrauto __attribute__((cleanup(_dstr_autofree))) dstr
static inline void _dstr_autofree(dstr* s)
{
    if (*s)
        dstrfree(*s);
}

/*
 * $(msg)
 * $(msg, cap)
 *
 * Shorthand alias for dstrnew.  Enabled by setting DSTR_SHORTCUT_ENABLED to 1
 * in dstr_options.h.
 */
#if DSTR_SHORTCUT_ENABLED
#define GET_DOLLAR(_1, _2, NAME, ...) NAME
#define $(...) GET_DOLLAR(__VA_ARGS__, $_custom, $_base)(__VA_ARGS__)
#define $_base(str) dstrnew_base(str)
#define $_custom(str, cap) dstrnew_custom(str, cap)
#endif

#endif
