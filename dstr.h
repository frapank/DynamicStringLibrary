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
#if !defined(__x86_64__) && !defined(_M_X64) && \
    !defined(__i386__) && !defined(_M_IX86) && \
    !defined(__aarch64__) && !defined(__arm__)
#error "DynamicStringLibrary supports only common architectures: x86, x86-64, ARM, ARM64"
#endif

#include <stddef.h>
#include <stdbool.h>

#define DSTR_SHORTCUT_ENABLED 0 /* 1 enabled | 0 disabled */

#define GET_DSTRNEW(_1, _2, NAME, ...) NAME
#define dstrnew(...) \
    GET_DSTRNEW(__VA_ARGS__, dstrnew_custom, dstrnew_base)(__VA_ARGS__)

#define GET_DSTRPUSH(_1,_2,_3,NAME,...) NAME
#define dstrpush(...) \
    GET_DSTRPUSH(__VA_ARGS__, dstrpush_custom, dstrpush_base)(__VA_ARGS__)

#define GET_DSTRCAT(_1,_2,_3,NAME,...) NAME
#define dstrcat(...) \
    GET_DSTRCAT(__VA_ARGS__, dstrcat_custom, dstrcat_base)(__VA_ARGS__)

#define GET_DSTRAPPEND(_1,_2,_3,NAME,...) NAME
#define dstrappend(...) \
    GET_DSTRAPPEND(__VA_ARGS__, dstrappend_custom, dstrappend_base)(__VA_ARGS__)

#define W_UNUSED_RESULT \
    __attribute__((warn_unused_result))

typedef char* dstr;


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
 * dstrlen(s)  - number of characters in s, excluding the null terminator.
 * dstrcap(s)  - total allocated capacity in bytes.
 */
size_t dstrlen(dstr s) W_UNUSED_RESULT;
size_t dstrcap(dstr s) W_UNUSED_RESULT;

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
dstr dstrpush_base(dstr s1, const char c);
dstr dstrpush_custom(dstr s1, const char c, size_t cap);

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
 * dstrauto
 *
 * Storage-class attribute that calls dstrfree automatically when the variable
 * goes out of scope (GCC/Clang cleanup extension).
 *
 *   dstrauto s = dstrnew("hello");
 */
#define dstrauto __attribute__((cleanup(_dstr_autofree))) dstr
static inline void _dstr_autofree(dstr* s) { if (*s) dstrfree(*s); }

/*
 * $(msg)
 * $(msg, cap)
 *
 * Shorthand alias for dstrnew.  Enabled by setting DSTR_SHORTCUT_ENABLED to 1
 * in dstr_options.h.
 */
#if DSTR_SHORTCUT_ENABLED
    #define GET_DOLLAR(_1,_2,NAME,...) NAME
    #define $(...) \
        GET_DOLLAR(__VA_ARGS__, $_custom, $_base)(__VA_ARGS__)
    #define $_base(str) \
        dstrnew_base(str)
    #define $_custom(str, cap) \
        dstrnew_custom(str, cap)
#endif


#endif
