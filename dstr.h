#ifndef DSTR_H
#define DSTR_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <stdbool.h>

// Public
#define dstrlen(x) _Generic((x), \
    dstr: dstrlen_str, \
    dstrhd: dstrlen_dstrhd, \
    dstrhdp: dstrlen_dstrhdp \
)(x)

struct dstrhd {
    unsigned int len;
    char buf[];
};

typedef struct dstrhd dstrhd;
typedef struct dstrhd* dstrhdp;
typedef char* dstr;

static inline unsigned int dstrlen_str(dstr s);
static inline unsigned int dstrlen_dstrhd(struct dstrhd s);
static inline unsigned int dstrlen_dstrhdp(struct dstrhd* s);
static inline struct dstrhd* dstrfull(dstr s);
static void dstrdebug(dstr s);
static dstr dstrnew(const char* msg);
static void dstrfree(dstr s);

// Implementation
#ifdef DSTR_IMPLEMENTATION

static inline unsigned int dstrlen_str(dstr s){return dstrfull(s)->len-1;};
static inline unsigned int dstrlen_dstrhd(struct dstrhd s) {return s.len-1;};
static inline unsigned int dstrlen_dstrhdp(struct dstrhd* s) {return s->len-1;};

static inline struct dstrhd* dstrfull(dstr s) 
{
    return (struct dstrhd*)((char*)s - offsetof(struct dstrhd, buf));
};

static void dstrdebug(dstr s)
{
    bool endchar = (s[dstrlen(s)-2] == '\n') ? 1 : 0;
    printf(endchar ? "[String]: %s" : "[String]: %s\n", s);
    printf("[Len]: %u\n", dstrlen(s));
    printf("[Addr]: %p\n", s);
}

static dstr dstrcat(dstr s, const dstr cs)
{
    dstrhd* hd = dstrfull(s); 
    dstrhd* chd = dstrfull(cs); 

    unsigned int new_len = hd->len + chd->len - 1;
    unsigned int new_mem = sizeof(dstrhd) + new_len;

    dstrhd* tmp = realloc(hd, new_mem);
    if(!tmp) return NULL;

    memcpy(((char*)tmp->buf + tmp->len -1), chd->buf, chd->len);
    tmp->len = new_len;

    return tmp->buf;
}

static dstr dstrnew(const char* msg)
{
    unsigned int s_len = strlen(msg) + 1;

    struct dstrhd* hd = malloc((sizeof(struct dstrhd)) + (s_len));
    if(!hd)
        return NULL;

    hd->len = s_len;
    dstr s_ret = hd->buf;

    memcpy(s_ret, msg, s_len);
    
    return s_ret;
}

static void dstrfree(dstr s)
{
    struct dstrhd* strhd = dstrfull(s);
    free(strhd);
}

#endif

#endif
