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

inline dstr dstrdup(dstr s) NONNULL(1) W_UNUSED_RESULT;
inline void dstrclear(dstr s) NONNULL(1);
inline _Bool dstrcmp(dstr s1, dstr s2) NONNULL(1,2) W_UNUSED_RESULT;
dstr dstrreserve(dstr s, size_t new_cap) NONNULL(1) W_UNUSED_RESULT;

dstr dstrcat_base(dstr s1, const char* s2) NONNULL(1,2) W_UNUSED_RESULT;
dstr dstrcat_custom(dstr s1, const char* s2, size_t cap) NONNULL(1,2) W_UNUSED_RESULT;

dstr dstrappend_base(dstr s1, const dstr s2) NONNULL(1,2) W_UNUSED_RESULT;
dstr dstrappend_custom(dstr s1, const dstr s2, size_t cap) NONNULL(1,2) W_UNUSED_RESULT;

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
#define DSTRGETTYPE(s) ((uint8_t)((s)[-1]))
#define DSTR_TODO \
    do { \
        fprintf(stderr, "dstr: function not implemented yet"); \
        exit(1); \
    } while (0)

static inline enum dstrhd_type _dstr_type_by_size(size_t cap) 
{
    if (cap <= UINT8_MAX) return DSTRHD_TYPE_8;
    if (cap <= UINT16_MAX) return DSTRHD_TYPE_16;
    if (cap <= UINT32_MAX) return DSTRHD_TYPE_32;
    return DSTRHD_TYPE_64;
}

static inline size_t _dstr_size_by_type(enum dstrhd_type type)
{
    switch(type) {
        case DSTRHD_TYPE_8: 
            return sizeof(struct dstrhd8);
        case DSTRHD_TYPE_16: 
            return sizeof(struct dstrhd16);
        case DSTRHD_TYPE_32: 
            return sizeof(struct dstrhd32);
        case DSTRHD_TYPE_64: 
            return sizeof(struct dstrhd64);
        default:
            return 0;
    }
}

static inline void _dstr_set_len(void* hd,
                                 size_t new_len,
                                 enum dstrhd_type type)
{
    switch(type) {
        case DSTRHD_TYPE_8:
            ((struct dstrhd8*)hd)->len = (uint8_t)new_len;
            return;

        case DSTRHD_TYPE_16:
            ((struct dstrhd16*)hd)->len = (uint16_t)new_len;
            return;

        case DSTRHD_TYPE_32:
            ((struct dstrhd32*)hd)->len = (uint32_t)new_len;
            return;

        case DSTRHD_TYPE_64:
            ((struct dstrhd64*)hd)->len = (uint64_t)new_len;
            return;
    }
}

static inline void _dstr_set_cap(void* hd,
                                 size_t new_cap,
                                 enum dstrhd_type type)
{
    switch(type) {
        case DSTRHD_TYPE_8:
            ((struct dstrhd8*)hd)->cap = (uint8_t)new_cap;
            return;

        case DSTRHD_TYPE_16:
            ((struct dstrhd16*)hd)->cap = (uint16_t)new_cap;
            return;

        case DSTRHD_TYPE_32:
            ((struct dstrhd32*)hd)->cap = (uint32_t)new_cap;
            return;

        case DSTRHD_TYPE_64:
            ((struct dstrhd64*)hd)->cap = (uint64_t)new_cap;
            return;
    }
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

// strdup
dstr dstrdup(dstr s)
{
    if (!s) return NULL;

    enum dstrhd_type t = DSTRGETTYPE(s);
    size_t len = dstrlen(s);
    size_t cap = dstrcap(s);

    void* old_hd = NULL;
    switch (t) {
        case DSTRHD_TYPE_8:  old_hd = DSTRGETHDR(8, s);  break;
        case DSTRHD_TYPE_16: old_hd = DSTRGETHDR(16, s); break;
        case DSTRHD_TYPE_32: old_hd = DSTRGETHDR(32, s); break;
        case DSTRHD_TYPE_64: old_hd = DSTRGETHDR(64, s); break;
    }

    if (!old_hd) return NULL;

    size_t hd_size = _dstr_size_by_type(t);
    void* new_hd = malloc(hd_size + cap);
    if (!new_hd) return NULL;

    _dstr_set_len(new_hd, len, t);
    _dstr_set_cap(new_hd, cap, t);

    *((uint8_t*)new_hd + hd_size - 1) = (uint8_t)t;

    memcpy((char*)new_hd + hd_size, s, len + 1);

    return (char*)new_hd + hd_size;
}

// strclear
void dstrclear(dstr s)
{
    enum dstrhd_type t = DSTRGETTYPE(s);
    void* hd = NULL;

    switch(t) {
        case DSTRHD_TYPE_8: hd = DSTRGETHDR(8, s); break;
        case DSTRHD_TYPE_16: hd = DSTRGETHDR(16, s); break;
        case DSTRHD_TYPE_32: hd = DSTRGETHDR(32, s); break;
        case DSTRHD_TYPE_64: hd = DSTRGETHDR(64, s); break;
    }

    if (!hd) return;

    _dstr_set_len(hd, 0, t);

    size_t cap = dstrcap(s);
    memset(s, 0, cap);
}

// strreserve
dstr dstrreserve(dstr s, size_t new_cap)
{
    size_t s_len = dstrlen(s);

    if (new_cap <= dstrcap(s))
        return s;

    if (new_cap < s_len + 1)
        new_cap = s_len + 1;

    enum dstrhd_type old_type = DSTRGETTYPE(s);
    enum dstrhd_type new_type = _dstr_type_by_size(new_cap);

    size_t old_hd_size = _dstr_size_by_type(old_type);
    size_t new_hd_size = _dstr_size_by_type(new_type);

    void* old_hd = (char*)s - old_hd_size;

    if(old_type == new_type) {
        void* new_hd = realloc(old_hd, new_hd_size + new_cap);
        if(!new_hd) 
            return NULL;

        s = (char*)new_hd + new_hd_size;

        _dstr_set_len(new_hd, s_len, new_type);
        _dstr_set_cap(new_hd, new_cap, new_type);

        return s;
    }  
    
    void* new_hd = malloc(new_hd_size + new_cap);
    if (!new_hd) 
        return NULL;

    _dstr_set_len(new_hd, s_len, new_type);
    _dstr_set_cap(new_hd, new_cap, new_type);

    *((char*)new_hd + new_hd_size-1) = (uint8_t)new_type;

    char* new_buf = (char*)new_hd + new_hd_size;
    memcpy(new_buf, s, s_len + 1);

    free(old_hd);

    return new_buf;
}

// strcmp
inline _Bool dstrcmp(dstr s1, dstr s2)
{
    if (dstrlen(s1) != dstrlen(s2))
        return 0;

    return memcmp(s1, s2, dstrlen(s1)) == 0;
}

// strcat & strappend
static dstr _dstr_concat_impl(dstr s1, size_t s1_len,
                               const char* s2, size_t s2_len,
                               size_t cap)
{
    size_t new_len = s1_len + s2_len;
    s1 = dstrreserve(s1, cap);
    if (!s1) return NULL;

    uint8_t hdr_type = DSTRGETTYPE(s1);
    void* hd = NULL;
    switch (hdr_type) {
        case DSTRHD_TYPE_8:  hd = DSTRGETHDR(8,  s1); break;
        case DSTRHD_TYPE_16: hd = DSTRGETHDR(16, s1); break;
        case DSTRHD_TYPE_32: hd = DSTRGETHDR(32, s1); break;
        case DSTRHD_TYPE_64: hd = DSTRGETHDR(64, s1); break;
    }
    memcpy(s1 + s1_len, s2, s2_len);
    s1[new_len] = '\0';
    _dstr_set_len(hd, new_len, hdr_type);
    return s1;
}

// strcat
dstr dstrcat_base(dstr s1, const char* s2)
{
    if (!s1 || !s2) return s1;
    size_t s1_len = dstrlen(s1);
    size_t s2_len = strlen(s2);
    return _dstr_concat_impl(s1, s1_len, s2, s2_len, s1_len + s2_len);
}

dstr dstrcat_custom(dstr s1, const char* s2, size_t cap)
{
    if (!s1 || !s2) return s1;
    return _dstr_concat_impl(s1, dstrlen(s1), s2, strlen(s2), cap);
}

// strappend
dstr dstrappend_base(dstr s1, const dstr s2)
{
    if (!s1 || !s2) return s1;
    size_t s1_len = dstrlen(s1);
    size_t s2_len = dstrlen(s2);
    return _dstr_concat_impl(s1, s1_len, s2, s2_len, s1_len + s2_len);
}

dstr dstrappend_custom(dstr s1, const dstr s2, size_t cap)
{
    if (!s1 || !s2) return s1;
    return _dstr_concat_impl(s1, dstrlen(s1), s2, dstrlen(s2), cap);
}

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
