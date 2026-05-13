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
#error "DynamicStringLibrary supporto only C11+ versions"
#endif
#if !defined(__x86_64__) && !defined(_M_X64) && \
    !defined(__i386__) && !defined(_M_IX86) && \
    !defined(__aarch64__) && !defined(__arm__)
#error "DynamicStringLibrary supports only common architectures: x86, x86-64, ARM, ARM64"
#endif

#include <stdlib.h>
#include <string.h>
#include <stddef.h>

/* Public */
#define DSTR_MIN_ALLOC_CAP 10

#define GET_DSTRNEW(_1, _2, NAME, ...) NAME
#define dstrnew(...) \
    GET_DSTRNEW(__VA_ARGS__, dstrnew_custom, dstrnew_base)(__VA_ARGS__)

#define GET_DSTRCAT(_1,_2,_3,NAME,...) NAME
#define dstrcat(...) \
    GET_DSTRCAT(__VA_ARGS__, dstrcat_custom, dstrcat_base)(__VA_ARGS__)

#define GET_DSTRAPPEND(_1,_2,_3,NAME,...) NAME
#define dstrappend(...) \
    GET_DSTRAPPEND(__VA_ARGS__, dstrappend_custom, dstrappend_base)(__VA_ARGS__)

struct dstrhd;

typedef struct dstrhd dstrhd;
typedef char* dstr;

// Functions
static inline struct dstrhd* dstrfull(dstr s) 
    __attribute__((warn_unused_result, always_inline));

static inline size_t dstrlen(dstr s)
    __attribute__((pure, nonnull(1), warn_unused_result));

static inline dstr dstrdup(dstr s)
    __attribute__((nonnull(1), warn_unused_result));

static inline void dstrclear(dstr s)
    __attribute__((always_inline, nonnull(1)));

static dstr dstrreserve(dstr s, size_t cap)
    __attribute__((nonnull(1), warn_unused_result));

static inline _Bool dstrcmp(dstr s1, dstr s2)
    __attribute__((always_inline, nonnull(1,2), warn_unused_result));


static dstr dstrcat_base(dstr s, const char* cs)
    __attribute__((nonnull(1,2), warn_unused_result));
static dstr dstrcat_custom(dstr s, const char* cs, size_t cap)
    __attribute__((nonnull(1,2), warn_unused_result));

static dstr dstrappend_base(dstr s, const dstr cs)
    __attribute__((nonnull(1,2), warn_unused_result));
static dstr dstrappend_custom(dstr s, const dstr cs, size_t cap)
    __attribute__((nonnull(1,2), warn_unused_result));


static dstr dstrnew_base(const char* msg)
    __attribute__((nonnull(1), warn_unused_result));
static dstr dstrnew_custom(const char* msg, size_t cap)
    __attribute__((nonnull(1), warn_unused_result));

static void dstrfree(dstr s)
    __attribute__((nonnull(1)));

/* Implementation */
#ifdef DSTR_IMPLEMENTATION

struct dstrhd {
    size_t len;
    size_t cap;
    char buf[];
};

// Shortcut
#ifdef DSTR_SHORTCUT
#   define GET_DOLLAR(_1,_2,NAME,...) NAME
#   define $(...) \
       GET_DOLLAR(__VA_ARGS__, $_custom, $_base)(__VA_ARGS__)
#   define $_base(str) \
       dstrnew_base(str)
#   define $_custom(str, cap) \
       dstrnew_custom(str, cap)
#endif

// Autofree
#define dstrauto __attribute__((cleanup(_dstr_autofree))) dstr

static inline void _dstr_autofree(dstr* s)
{
    if (*s) dstrfree(*s);
}

// strlen
static inline size_t dstrlen(dstr s)
{
    return dstrfull(s)->len;
};

// strdup
static inline dstr dstrdup(dstr s) {
    dstrhd* hd = dstrfull(s);
    return dstrnew_custom(hd->buf, hd->cap-1);
}

// strclear
static void dstrclear(dstr s)
{
    dstrhd* hd = dstrfull(s);
    if(hd->len == 0) return;
    memset(hd->buf, 0, hd->cap);
    hd->len = 0;
}

// strfull
static inline struct dstrhd* dstrfull(dstr s) 
{
    return (struct dstrhd*)((char*)s - offsetof(struct dstrhd, buf));
};

// strresize
static dstr dstrreserve(dstr s, size_t cap)
{
    dstrhd* hd = dstrfull(s);

    if(hd->cap >= cap)
        return hd->buf;

    dstrhd* tmp = realloc(hd, sizeof(dstrhd) + cap);
    if(!tmp)
        return NULL;

    size_t old_cap = tmp->cap;
    tmp->cap = cap;
    memset(tmp->buf + old_cap, 0, cap - old_cap);
    return tmp->buf;
}

// strcmp
static inline _Bool dstrcmp(dstr s1, dstr s2)
{
    dstrhd* hd1 = dstrfull(s1);
    dstrhd* hd2 = dstrfull(s2);

    if(hd1->len != hd2->len) 
        return 0;

    return memcmp(hd1->buf, hd2->buf, hd1->len) == 0;
}

// strcat & strappend helper
static inline char* _dstr_cat_append_helper(dstrhd* hd, const char* s, 
        size_t s_len, size_t new_len, size_t hint_cap)
{
    dstrhd* tmp = hd;

    if (hd->cap < new_len+1) {
        size_t new_cap = tmp->cap * 2;
        if (new_cap < new_len+1) new_cap = new_len+1;
        if (hint_cap > new_cap) new_cap = hint_cap;

        tmp = realloc(hd, sizeof(dstrhd) + new_cap);
        if (!tmp) return NULL;

        tmp->cap = new_cap;
    }

    memcpy(tmp->buf + tmp->len, s, s_len);
    tmp->buf[new_len] = '\0';
    tmp->len = new_len;
    return tmp->buf;
}

// strcat
static dstr dstrcat_base(dstr s, const char* cs)
{
    dstrhd* hd = dstrfull(s);
    size_t cs_len = strlen(cs);
    size_t new_len = hd->len + cs_len;

    return _dstr_cat_append_helper(hd, cs, cs_len, new_len, 0);
}

static dstr dstrcat_custom(dstr s, const char* cs, size_t cap)
{
    dstrhd* hd = dstrfull(s);
    size_t cs_len = strlen(cs);
    size_t new_len = hd->len + cs_len;

    return _dstr_cat_append_helper(hd, cs, cs_len, new_len, cap);
}

// strappend
static dstr dstrappend_base(dstr s, const dstr cs)
{
    dstrhd* hd = dstrfull(s);
    dstrhd* chd = dstrfull(cs);
    size_t new_len = hd->len + chd->len;

    return _dstr_cat_append_helper(hd, chd->buf, chd->len, new_len, 0);
}

static dstr dstrappend_custom(dstr s, const dstr cs, size_t cap)
{
    dstrhd* hd = dstrfull(s);
    dstrhd* chd = dstrfull(cs);
    size_t new_len = hd->len + chd->len;

    return _dstr_cat_append_helper(hd, chd->buf, chd->len, new_len, cap);
}

// strnew
static dstr dstrnew_base(const char* msg)
{
    size_t s_len = strlen(msg);
    size_t alloc_size = s_len+1;

    if(alloc_size < DSTR_MIN_ALLOC_CAP) 
        alloc_size = DSTR_MIN_ALLOC_CAP;

    struct dstrhd* hd = malloc((sizeof(struct dstrhd)) + (alloc_size));
    if(!hd) return NULL;

    hd->cap = alloc_size;
    hd->len = s_len;
    memcpy(hd->buf, msg, s_len+1);
    return hd->buf;
}

static dstr dstrnew_custom(const char* msg, size_t cap)
{
    size_t s_len = strlen(msg);
    size_t alloc_size = cap;

    if(s_len > cap) 
        alloc_size = s_len+1;

    struct dstrhd* hd = malloc((sizeof(struct dstrhd)) + (alloc_size));
    if(!hd) return NULL;

    hd->cap = alloc_size;
    hd->len = s_len;
    memcpy(hd->buf, msg, s_len+1);
    return hd->buf;
}

// strfree
static void dstrfree(dstr s)
{
    struct dstrhd* strhd = dstrfull(s);
    free(strhd);
}

#endif
#endif
