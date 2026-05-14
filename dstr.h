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
#include <stdint.h>

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

#define NONNULL(...) \
    __attribute__((nonnull(__VA_ARGS__)))

#define W_UNUSED_RESULT \
    __attribute__((warn_unused_result))

typedef char* dstr;

// Functions
#ifdef DSTR_SHORTCUT
    inline void _dstr_autofree(dstr* s) NONNULL(1);
#endif

size_t dstrlen(dstr s) NONNULL(1) W_UNUSED_RESULT;
size_t dstrcap(dstr s) NONNULL(1) W_UNUSED_RESULT;

//inline dstr dstrdup(dstr s) NONNULL(1) W_UNUSED_RESULT;
//inline void dstrclear(dstr s) NONNULL(1);
//inline _Bool dstrcmp(dstr s1, dstr s2) NONNULL(1,2) W_UNUSED_RESULT;
//dstr dstrreserve(dstr s, size_t cap) NONNULL(1) W_UNUSED_RESULT;

//dstr dstrcat_base(dstr s, const char* cs) NONNULL(1,2) W_UNUSED_RESULT;
//dstr dstrcat_custom(dstr s, const char* cs, size_t cap) NONNULL(1,2) W_UNUSED_RESULT;

//dstr dstrappend_base(dstr s, const dstr cs) NONNULL(1,2) W_UNUSED_RESULT;
//dstr dstrappend_custom(dstr s, const dstr cs, size_t cap) NONNULL(1,2) W_UNUSED_RESULT;

dstr dstrnew_base(const char* msg) NONNULL(1) W_UNUSED_RESULT;
dstr dstrnew_custom(const char* msg, size_t cap) NONNULL(1) W_UNUSED_RESULT;

void dstrfree(dstr s) NONNULL(1);

/* Implementation */
#ifdef DSTR_IMPLEMENTATION

enum dstrhd_type {
    DSTRHD_TYPE_8 = 0,
    DSTRHD_TYPE_16 = 1,
    DSTRHD_TYPE_32 = 2,
    DSTRHD_TYPE_64 = 3
};

struct __attribute__((packed)) dstrhd8 {
    uint8_t len;
    uint8_t cap; // 255
    uint8_t type;
    char buf[];
};

struct __attribute__((packed)) dstrhd16 {
    uint16_t len;
    uint16_t cap; // 65,535
    uint8_t type;
    char buf[];
};

struct __attribute__((packed)) dstrhd32 {
    uint32_t len;
    uint32_t cap; // 4,294,967,295
    uint8_t type;
    char buf[];
};

struct __attribute__((packed)) dstrhd64 {
    uint64_t len;
    uint64_t cap; // 18,446,744,073,709,551,615
    uint8_t type;
    char buf[];
};

// Utils
#define DSTRGETHDR(n,s) \
    ((struct dstrhd##n *)((char*)(s) - sizeof(struct dstrhd##n)))
#define DSTRGETTYPE(s) \
    s[-1]

static inline enum dstrhd_type _dstr_type_by_size(size_t cap) 
{
    if (cap <= UINT8_MAX) return DSTRHD_TYPE_8;
    if (cap <= UINT16_MAX) return DSTRHD_TYPE_16;
    if (cap <= UINT32_MAX) return DSTRHD_TYPE_32;
    return DSTRHD_TYPE_64;
}

static inline dstr _dstr_change_header(dstr s, enum dstrhd_type t)
{
          
}

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

void _dstr_autofree(dstr* s)
{
    if (*s) dstrfree(*s);
}

// strlen
size_t dstrlen(dstr s)
{
    enum dstrhd_type t = DSTRGETTYPE(s);
    switch(t) {
        case DSTRHD_TYPE_8: {
            return DSTRGETHDR(8, s)->len;
        }
        case DSTRHD_TYPE_16: {
            return DSTRGETHDR(16, s)->len;
        }
        case DSTRHD_TYPE_32: {
            return DSTRGETHDR(32, s)->len;
        }
        case DSTRHD_TYPE_64: {
            return DSTRGETHDR(64, s)->len;
        }
    }
    return 0;
};

// dstrcap
size_t dstrcap(dstr s)
{
    enum dstrhd_type t = DSTRGETTYPE(s);
    switch(t) {
        case DSTRHD_TYPE_8: {
            return DSTRGETHDR(8, s)->cap;
        }
        case DSTRHD_TYPE_16: {
            return DSTRGETHDR(16, s)->cap;
        }
        case DSTRHD_TYPE_32: {
            return DSTRGETHDR(32, s)->cap;
        }
        case DSTRHD_TYPE_64: {
            return DSTRGETHDR(64, s)->cap;
        }
    }
    return 0;
}

//// strdup
//dstr dstrdup(dstr s) {
//    //dstrhd* hd = dstrfull(s);
//    return dstrnew_custom(hd->buf, hd->cap-1);
//}
//
//// strclear
//void dstrclear(dstr s)
//{
//    //dstrhd* hd = dstrfull(s);
//    if(hd->len == 0) return;
//    memset(hd->buf, 0, hd->cap);
//    hd->len = 0;
//}
//
//// strresize
//dstr dstrreserve(dstr s, size_t cap)
//{
//    //dstrhd* hd = dstrfull(s);
//
//    if(hd->cap >= cap)
//        return hd->buf;
//
//    dstrhd* tmp = realloc(hd, sizeof(dstrhd) + cap);
//    if(!tmp)
//        return NULL;
//
//    size_t old_cap = tmp->cap;
//    tmp->cap = cap;
//    memset(tmp->buf + old_cap, 0, cap - old_cap);
//    return tmp->buf;
//}
//
//// strcmp
//inline _Bool dstrcmp(dstr s1, dstr s2)
//{
//    //dstrhd* hd1 = dstrfull(s1);
//    //dstrhd* hd2 = dstrfull(s2);
//
//    if(hd1->len != hd2->len) 
//        return 0;
//
//    return memcmp(hd1->buf, hd2->buf, hd1->len) == 0;
//}
//
//// strcat & strappend helper
//static inline char* _dstr_cat_append_helper(dstrhd* hd, const char* s, 
//        size_t s_len, size_t new_len, size_t hint_cap)
//{
//    dstrhd* tmp = hd;
//
//    if (hd->cap < new_len+1) {
//        size_t new_cap = tmp->cap * 2;
//        if (new_cap < new_len+1) new_cap = new_len+1;
//        if (hint_cap > new_cap) new_cap = hint_cap;
//
//        tmp = realloc(hd, sizeof(dstrhd) + new_cap);
//        if (!tmp) return NULL;
//
//        tmp->cap = new_cap;
//    }
//
//    memcpy(tmp->buf + tmp->len, s, s_len);
//    tmp->buf[new_len] = '\0';
//    tmp->len = new_len;
//    return tmp->buf;
//}
//
//// strcat
//dstr dstrcat_base(dstr s, const char* cs)
//{
//    //dstrhd* hd = dstrfull(s);
//    size_t cs_len = strlen(cs);
//    size_t new_len = hd->len + cs_len;
//
//    return _dstr_cat_append_helper(hd, cs, cs_len, new_len, 0);
//}
//
//dstr dstrcat_custom(dstr s, const char* cs, size_t cap)
//{
//    //dstrhd* hd = dstrfull(s);
//    size_t cs_len = strlen(cs);
//    size_t new_len = hd->len + cs_len;
//
//    return _dstr_cat_append_helper(hd, cs, cs_len, new_len, cap);
//}
//
//// strappend
//dstr dstrappend_base(dstr s, const dstr cs)
//{
//    //dstrhd* hd = dstrfull(s);
//    //dstrhd* chd = dstrfull(cs);
//    size_t new_len = hd->len + chd->len;
//
//    return _dstr_cat_append_helper(hd, chd->buf, chd->len, new_len, 0);
//}
//
//dstr dstrappend_custom(dstr s, const dstr cs, size_t cap)
//{
//    //dstrhd* hd = dstrfull(s);
//    //dstrhd* chd = dstrfull(cs);
//    size_t new_len = hd->len + chd->len;
//
//    return _dstr_cat_append_helper(hd, chd->buf, chd->len, new_len, cap);
//}

// strnew
static dstr _dstrnew_allocator(enum dstrhd_type t, const char* msg, size_t s_len, size_t alloc_size)
{
    switch (t) {
        case DSTRHD_TYPE_8: {
            struct dstrhd8* hd = malloc(sizeof(*hd) + alloc_size);
            if (!hd) return NULL;

            hd->cap = (uint8_t)alloc_size;
            hd->len = (uint8_t)s_len;
            hd->type = DSTRHD_TYPE_8;

            memcpy(hd->buf, msg, s_len + 1);
            return (dstr)hd->buf;
        }

        case DSTRHD_TYPE_16: {
            struct dstrhd16* hd = malloc(sizeof(*hd) + alloc_size);
            if (!hd) return NULL;

            hd->cap = (uint16_t)alloc_size;
            hd->len = (uint16_t)s_len;
            hd->type = DSTRHD_TYPE_16;

            memcpy(hd->buf, msg, s_len + 1);
            return (dstr)hd->buf;
        }

        case DSTRHD_TYPE_32: {
            struct dstrhd32* hd = malloc(sizeof(*hd) + alloc_size);
            if (!hd) return NULL;

            hd->cap = (uint32_t)alloc_size;
            hd->len = (uint32_t)s_len;
            hd->type = DSTRHD_TYPE_32;

            memcpy(hd->buf, msg, s_len + 1);
            return (dstr)hd->buf;
        }

        case DSTRHD_TYPE_64: {
            struct dstrhd64* hd = malloc(sizeof(*hd) + alloc_size);
            if (!hd) return NULL;

            hd->cap = (uint64_t)alloc_size;
            hd->len = (uint64_t)s_len;
            hd->type = DSTRHD_TYPE_64;

            memcpy(hd->buf, msg, s_len + 1);
            return (dstr)hd->buf;
        }
    }
    return NULL;
}

dstr dstrnew_base(const char* msg)
{
    size_t s_len = strlen(msg);
    size_t alloc_size = s_len+1;

    enum dstrhd_type t = _dstr_type_by_size(alloc_size);

    return _dstrnew_allocator(t, msg, s_len, alloc_size);
}

dstr dstrnew_custom(const char* msg, size_t cap)
{
    size_t s_len = strlen(msg);
    size_t alloc_size = (s_len > cap) ? (s_len + 1) : cap;

    enum dstrhd_type t = _dstr_type_by_size(alloc_size);

    return _dstrnew_allocator(t, msg, s_len, alloc_size);

}

// strfree
void dstrfree(dstr s)
{
    enum dstrhd_type t = s[-1];

    switch(t) {
        case DSTRHD_TYPE_8: {
            free(DSTRGETHDR(8, s));
            return;
        }
        case DSTRHD_TYPE_16: {
            free(DSTRGETHDR(16, s));
            return;
        }
        case DSTRHD_TYPE_32: {
            free(DSTRGETHDR(32, s));
            return;
        }
        case DSTRHD_TYPE_64: {
            free(DSTRGETHDR(64, s));
            return;
        }

    }
}

#endif
#endif
