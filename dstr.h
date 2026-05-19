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

/* Public */
#define GET_DSTRNEW(_1, _2, NAME, ...) NAME
#define dstrnew(...) \
    GET_DSTRNEW(__VA_ARGS__, dstrnew_custom, dstrnew_base)(__VA_ARGS__)

#define GET_DSTRCAT(_1,_2,_3,NAME,...) NAME
#define dstrcat(...) \
    GET_DSTRCAT(__VA_ARGS__, dstrcat_custom, dstrcat_base)(__VA_ARGS__)

#define GET_DSTRAPPEND(_1,_2,_3,NAME,...) NAME
#define dstrappend(...) \
    GET_DSTRAPPEND(__VA_ARGS__, dstrappend_custom, dstrappend_base)(__VA_ARGS__)

#define W_UNUSED_RESULT \
    __attribute__((warn_unused_result))

typedef char* dstr;

// Functions
size_t dstrlen(dstr s) W_UNUSED_RESULT;
size_t dstrcap(dstr s) W_UNUSED_RESULT;

dstr dstrdup(dstr s) W_UNUSED_RESULT;
void dstrclear(dstr s);
void dstrzero(dstr s);
bool dstrequal(dstr s1, dstr s2) W_UNUSED_RESULT;
dstr dstrreserve(dstr s, size_t new_cap) W_UNUSED_RESULT;

dstr dstrcat_base(dstr s1, const char* s2) W_UNUSED_RESULT;
dstr dstrcat_custom(dstr s1, const char* s2, size_t cap) W_UNUSED_RESULT;

dstr dstrappend_base(dstr s1, const dstr s2) W_UNUSED_RESULT;
dstr dstrappend_custom(dstr s1, const dstr s2, size_t cap) W_UNUSED_RESULT;

dstr dstrnew_base(const char* msg) W_UNUSED_RESULT;
dstr dstrnew_custom(const char* msg, size_t cap) W_UNUSED_RESULT;

void dstrfree(dstr s);

// Shortcut
#ifdef DSTR_SHORTCUT
    #define GET_DOLLAR(_1,_2,NAME,...) NAME
    #define $(...) \
        GET_DOLLAR(__VA_ARGS__, $_custom, $_base)(__VA_ARGS__)
    #define $_base(str) \
        dstrnew_base(str)
    #define $_custom(str, cap) \
        dstrnew_custom(str, cap)
#endif

#define dstrauto __attribute__((cleanup(_dstr_autofree))) dstr
static inline void _dstr_autofree(dstr* s)
{
    if (*s) dstrfree(*s);
}

#endif

