/*
 * Copyright (c) 2026 Francesco Carbone (@frapank)
 * SPDX-License-Identifier: BSD-2-Clause
 */

#ifndef DSTR_H
#define DSTR_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <stdbool.h>

/* Public */
#define DSTR_MIN_ALLOC_CAP 10

#define dstrlen(x) _Generic((x), \
    dstr: dstrlen_str, \
    dstrhd: dstrlen_dstrhd, \
    dstrhdp: dstrlen_dstrhdp \
)(x)

#define dstrdup(x) _Generic((x), \
    dstr: dstrdup_str, \
    dstrhd: dstrdup_dstrhd, \
    dstrhdp: dstrdup_dstrhdp \
)(x)

#define dstrclear(x) _Generic((x), \
    dstr: dstrclear_str, \
    dstrhdp: dstrclear_dstrhdp \
)(x)

#define GET_DSTRNEW(_1, _2, NAME, ...) NAME
#define dstrnew(...) \
    GET_DSTRNEW(__VA_ARGS__, dstrnew_custom, dstrnew_base)(__VA_ARGS__)

#define GET_DSTRCAT(_1,_2,_3,NAME,...) NAME
#define dstrcat(...) \
    GET_DSTRCAT(__VA_ARGS__, dstrcat_custom, dstrcat_base)(__VA_ARGS__)

struct dstrhd {
    unsigned int len;
    unsigned int cap;
    char buf[];
};

typedef struct dstrhd dstrhd;
typedef struct dstrhd* dstrhdp;
typedef char* dstr;

// Functions
static inline struct dstrhd* dstrfull(dstr s) 
    __attribute__((pure, warn_unused_result, always_inline));

static inline unsigned int dstrlen_str(dstr s)
    __attribute__((pure, nonnull(1)));
static inline unsigned int dstrlen_dstrhd(struct dstrhd s)
    __attribute__((pure));
static inline unsigned int dstrlen_dstrhdp(struct dstrhd* s)
    __attribute__((pure, nonnull(1)));

static inline dstr dstrdup_str(dstr s)
    __attribute__((nonnull(1), malloc, warn_unused_result));
static inline dstr dstrdup_dstrhd(struct dstrhd s)
    __attribute__((malloc, warn_unused_result));
static inline dstr dstrdup_dstrhdp(struct dstrhd* s)
    __attribute__((nonnull(1), malloc, warn_unused_result));

static inline void dstrclear_str(dstr s)
    __attribute__((nonnull(1), always_inline));
static void dstrclear_dstrhdp(struct dstrhd* s)
    __attribute__((nonnull(1)));

static dstr dstrcat_base(dstr s, const dstr cs)
    __attribute__((nonnull(1,2), warn_unused_result));
static dstr dstrcat_custom(dstr s, const dstr cs, unsigned int cap)
    __attribute__((nonnull(1,2), warn_unused_result));

static dstr dstrnew_base(const char* msg)
    __attribute__((nonnull(1), malloc, warn_unused_result, returns_nonnull));
static dstr dstrnew_custom(const char* msg, unsigned int cap)
    __attribute__((nonnull(1), malloc, warn_unused_result, returns_nonnull));

static void dstrfree(dstr s)
    __attribute__((nonnull(1)));

/* Implementation */
#ifdef DSTR_IMPLEMENTATION

// strlen
static inline unsigned int dstrlen_str(dstr s){return dstrfull(s)->len-1;};
static inline unsigned int dstrlen_dstrhd(struct dstrhd s) {return s.len-1;};
static inline unsigned int dstrlen_dstrhdp(struct dstrhd* s) {return s->len-1;};

// strdup
static inline dstr dstrdup_str(dstr s) {
    dstrhd* hd = dstrfull(s);
    return dstrnew_custom(hd->buf, hd->cap);
}
static inline dstr dstrdup_dstrhd(dstrhd s) {return dstrnew_custom(s.buf, s.cap);}
static inline dstr dstrdup_dstrhdp(dstrhdp s) {return dstrnew_custom(s->buf, s->cap);}

// strclear
static inline void dstrclear_str(dstr s) {dstrclear_dstrhdp(dstrfull(s));}
static void dstrclear_dstrhdp(struct dstrhd* s)
{
    if(!s || s->len <= 1) return;
    memset(s->buf, 0, s->len);
    s->len = 1;
}

// strfull
static inline struct dstrhd* dstrfull(dstr s) 
{
    return (struct dstrhd*)((char*)s - offsetof(struct dstrhd, buf));
};

// strcat
static dstr dstrcat_base(dstr s, const dstr cs)
{
    dstrhd* hd = dstrfull(s); 
    dstrhd* chd = dstrfull(cs); 

    unsigned int new_len = hd->len + chd->len - 1;

    dstrhd* tmp = hd;

    if(hd->cap < new_len) {
        unsigned int new_cap = tmp->cap * 2;
        if (new_cap < new_len) new_cap = new_len;
        
        tmp = realloc(hd, sizeof(dstrhd) + new_cap);
        if(!tmp) return NULL;

        tmp->cap = new_cap;
    }

    memcpy(tmp->buf + tmp->len - 1, chd->buf, chd->len);
    tmp->len = new_len;
    hd = tmp;

    return tmp->buf;
}

static dstr dstrcat_custom(dstr s, const dstr cs, unsigned int cap)
{
    dstrhd* hd = dstrfull(s); 
    dstrhd* chd = dstrfull(cs); 

    unsigned int new_len = hd->len + chd->len - 1;

    dstrhd* tmp = hd;

    if(hd->cap < new_len) {
        unsigned int new_cap = cap;
        if (new_cap < new_len) new_cap = new_len;
        
        tmp = realloc(hd, sizeof(dstrhd) + new_cap);
        if(!tmp) return NULL;

        tmp->cap = new_cap;
    }

    memcpy(tmp->buf + tmp->len - 1, chd->buf, chd->len);
    tmp->len = new_len;
    hd = tmp;

    return tmp->buf;
}

// strnew
static dstr dstrnew_base(const char* msg)
{
    unsigned int s_len = strlen(msg) + 1;
    unsigned int alloc_size = s_len;

    if(s_len < DSTR_MIN_ALLOC_CAP) 
        alloc_size = DSTR_MIN_ALLOC_CAP;

    struct dstrhd* hd = malloc((sizeof(struct dstrhd)) + (alloc_size));
    if(!hd)
        return NULL;

    hd->cap = alloc_size;
    hd->len = s_len;
    dstr s_ret = hd->buf;

    memcpy(s_ret, msg, s_len);
    
    return s_ret;
}

static dstr dstrnew_custom(const char* msg, unsigned int cap)
{
    unsigned int s_len = strlen(msg) + 1;
    unsigned int alloc_size = cap;

    if(s_len > cap) 
        alloc_size = s_len;

    struct dstrhd* hd = malloc((sizeof(struct dstrhd)) + (alloc_size));
    if(!hd)
        return NULL;

    hd->cap = alloc_size;
    hd->len = s_len;
    dstr s_ret = hd->buf;

    memcpy(s_ret, msg, s_len);

    return s_ret;
}

// strfree
static void dstrfree(dstr s)
{
    struct dstrhd* strhd = dstrfull(s);
    free(strhd);
}

#endif

#endif
