/*
 * Copyright (c) 2026 Francesco Carbone (@frapank)
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "dstr.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "dstr_options.h"

static inline enum dstrhd_type _dstr_type_by_size(size_t cap)
{
    if (cap <= UINT8_MAX)
        return DSTRHD_TYPE_8;
    if (cap <= UINT16_MAX)
        return DSTRHD_TYPE_16;
    if (cap <= UINT32_MAX)
        return DSTRHD_TYPE_32;
    return DSTRHD_TYPE_64;
}

static inline size_t _dstr_size_by_type(enum dstrhd_type type)
{
    switch (type) {
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
    switch (type) {
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
    switch (type) {
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

static inline void* _dstr_get_hdr_and_type(dstr s, enum dstrhd_type* t_out)
{
    if (!s)
        return NULL;

    enum dstrhd_type t = DSTRGETTYPE(s);

    if (t_out)
        *t_out = t;

    switch (t) {
        case DSTRHD_TYPE_8:
            return DSTRGETHDR(8, s);

        case DSTRHD_TYPE_16:
            return DSTRGETHDR(16, s);

        case DSTRHD_TYPE_32:
            return DSTRGETHDR(32, s);

        case DSTRHD_TYPE_64:
            return DSTRGETHDR(64, s);

        default:
            return NULL;
    }
}

static inline size_t _dstr_grow_cap(size_t old_cap, size_t needed)
{
    if (needed <= old_cap)
        return old_cap;

#if DSTR_GROWTH_ENABLED
    size_t grown = old_cap < 16 ? 16 : old_cap;

    if (grown <= SIZE_MAX / DSTR_GROWTH_PERCENT) {
        size_t increment = grown * DSTR_GROWTH_PERCENT / 100;
        if (grown <= SIZE_MAX - increment)
            grown += increment;
    }

    return grown > needed ? grown : needed;
#else
    return needed;
#endif
}

static inline ssize_t _dstr_find_from(const char* buf,
                                      size_t len,
                                      const char* needle,
                                      size_t nlen,
                                      size_t from)
{
    if (from > len || nlen > len - from)
        return -1;

    size_t remaining = len - from - nlen + 1;
    const char* cursor = buf + from;

    while (remaining > 0) {
        const char* hit = memchr(cursor, needle[0], remaining);
        if (!hit)
            return -1;
        if (memcmp(hit, needle, nlen) == 0)
            return hit - buf;
        remaining -= (size_t)(hit - cursor) + 1;
        cursor = hit + 1;
    }

    return -1;
}

static dstr _dstr_slice_alloc(const char* buf, size_t len)
{
    enum dstrhd_type t = _dstr_type_by_size(len + 1);
    size_t hd_size = _dstr_size_by_type(t);

    void* hd = DSTR_MALLOC(hd_size + len + 1);
    if (!hd)
        return NULL;

    _dstr_set_len(hd, len, t);
    _dstr_set_cap(hd, len + 1, t);
    *((uint8_t*)hd + hd_size - 1) = (uint8_t)t;

    char* buf_out = (char*)hd + hd_size;
    memcpy(buf_out, buf, len);
    buf_out[len] = '\0';

    return buf_out;
}

// strfind
ssize_t dstrfind(dstr s, const char* needle)
{
    if (!s || !needle || needle[0] == '\0')
        return -1;

    return _dstr_find_from(s, dstrlen(s), needle, strlen(needle), 0);
}

// dstrrange
void dstrrange(dstr s, ssize_t start, ssize_t end)
{
    enum dstrhd_type t;
    void* hd = _dstr_get_hdr_and_type(s, &t);
    if (!hd)
        return;

    ssize_t len = (ssize_t)dstrlen(s);
    if (len == 0)
        return;

    if (start < 0) {
        start += len;
        if (start < 0)
            start = 0;
    }
    if (end < 0) {
        end += len;
        if (end < 0)
            end = 0;
    }

    size_t newlen = (start > end) ? 0 : (size_t)(end - start) + 1;
    if (newlen != 0) {
        if (start >= len) {
            newlen = 0;
        } else if (end >= len) {
            end = len - 1;
            newlen = (size_t)(end - start) + 1;
        }
    }

    if (start && newlen)
        memmove(s, s + start, newlen);
    s[newlen] = '\0';
    _dstr_set_len(hd, newlen, t);
}

// dstrsplit
dstr* dstrsplit(dstr s, const char* delim, size_t* out_count)
{
    if (out_count)
        *out_count = 0;

    if (!s || !delim || delim[0] == '\0')
        return NULL;

    size_t len = dstrlen(s);
    size_t dlen = strlen(delim);

    size_t count = 1;
    size_t pos = 0;
    for (;;) {
        ssize_t idx = _dstr_find_from(s, len, delim, dlen, pos);
        if (idx < 0)
            break;
        count++;
        pos = (size_t)idx + dlen;
    }

    if (count > SIZE_MAX / sizeof(dstr))
        return NULL;

    dstr* parts = DSTR_MALLOC(count * sizeof(dstr));
    if (!parts)
        return NULL;

    size_t n = 0;
    pos = 0;
    for (;;) {
        ssize_t idx = _dstr_find_from(s, len, delim, dlen, pos);
        size_t piece_end = (idx < 0) ? len : (size_t)idx;

        parts[n] = _dstr_slice_alloc(s + pos, piece_end - pos);
        if (!parts[n]) {
            for (size_t i = 0; i < n; i++)
                dstrfree(parts[i]);
            DSTR_FREE(parts);
            return NULL;
        }
        n++;

        if (idx < 0)
            break;
        pos = piece_end + dlen;
    }

    if (out_count)
        *out_count = n;

    return parts;
}

// dstrsplitfree
void dstrsplitfree(dstr* parts, size_t count)
{
    if (!parts)
        return;

    for (size_t i = 0; i < count; i++)
        dstrfree(parts[i]);

    DSTR_FREE(parts);
}

// strdup
dstr dstrdup(dstr s)
{
    if (!s)
        return NULL;

    enum dstrhd_type t;
    const void* old_hd = _dstr_get_hdr_and_type(s, &t);
    if (!old_hd)
        return NULL;

    size_t len = dstrlen(s);
    size_t cap = dstrcap(s);
    size_t hd_size = _dstr_size_by_type(t);

    void* new_hd = DSTR_MALLOC(hd_size + cap);
    if (!new_hd)
        return NULL;

    _dstr_set_len(new_hd, len, t);
    _dstr_set_cap(new_hd, cap, t);
    *((uint8_t*)new_hd + hd_size - 1) = (uint8_t)t;

    memcpy((char*)new_hd + hd_size, s, len + 1);
    return (char*)new_hd + hd_size;
}

// dstrclear
void dstrclear(dstr s)
{
    enum dstrhd_type t;
    void* hd = _dstr_get_hdr_and_type(s, &t);

    if (!hd)
        return;

    _dstr_set_len(hd, 0, t);
    s[0] = '\0';
}

// dstrzero
void dstrzero(dstr s)
{
    enum dstrhd_type t;
    void* hd = _dstr_get_hdr_and_type(s, &t);

    if (!hd)
        return;

    _dstr_set_len(hd, 0, t);

    size_t cap = dstrcap(s);
    memset(s, 0, cap);
}

// dstrtolower
void dstrtolower(dstr s)
{
    if (!s)
        return;

    size_t len = dstrlen(s);
    for (size_t i = 0; i < len; i++) {
        unsigned char c = (unsigned char)s[i];
        if (c >= 'A' && c <= 'Z')
            s[i] = (char)(c + ('a' - 'A'));
    }
}

// dstrtoupper
void dstrtoupper(dstr s)
{
    if (!s)
        return;

    size_t len = dstrlen(s);
    for (size_t i = 0; i < len; i++) {
        unsigned char c = (unsigned char)s[i];
        if (c >= 'a' && c <= 'z')
            s[i] = (char)(c - ('a' - 'A'));
    }
}

// strreserve
dstr dstrreserve(dstr s, size_t new_cap)
{
    if (!s)
        return NULL;
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

    if (old_type == new_type) {
        void* new_hd = DSTR_REALLOC(old_hd, new_hd_size + new_cap);
        if (!new_hd)
            return NULL;

        s = (char*)new_hd + new_hd_size;

        _dstr_set_len(new_hd, s_len, new_type);
        _dstr_set_cap(new_hd, new_cap, new_type);

        *((uint8_t*)new_hd + new_hd_size - 1) = (uint8_t)new_type;

        return s;
    }

    void* new_hd = DSTR_MALLOC(new_hd_size + new_cap);
    if (!new_hd)
        return NULL;

    _dstr_set_len(new_hd, s_len, new_type);
    _dstr_set_cap(new_hd, new_cap, new_type);

    *((char*)new_hd + new_hd_size - 1) = (uint8_t)new_type;

    char* new_buf = (char*)new_hd + new_hd_size;
    memcpy(new_buf, s, s_len + 1);

    DSTR_FREE(old_hd);

    return new_buf;
}

// strcmp
bool dstrequal(dstr s1, dstr s2)
{
    if (!s1 || !s2)
        return false;
    size_t s1_len = dstrlen(s1);
    if (s1_len != dstrlen(s2))
        return false;

    return memcmp(s1, s2, s1_len) == 0;
}

// strcat & strappend
static dstr _dstr_concat_impl(dstr s1,
                              size_t s1_len,
                              const char* s2,
                              size_t s2_len,
                              size_t cap)
{
    if (s2_len > SIZE_MAX - s1_len)
        return NULL;
    size_t new_len = s1_len + s2_len;
    if (cap < new_len + 1)
        cap = new_len + 1;

    uintptr_t s1_addr = (uintptr_t)s1;
    uintptr_t s2_addr = (uintptr_t)s2;
    size_t s1_cap = dstrcap(s1);
    bool s2_aliases_s1 = s2_addr >= s1_addr && s2_addr - s1_addr < s1_cap;
    size_t s2_offset = s2_aliases_s1 ? (size_t)(s2_addr - s1_addr) : 0;

    dstr tmp = dstrreserve(s1, cap);
    if (!tmp)
        return NULL;
    s1 = tmp;

    if (s2_aliases_s1)
        s2 = s1 + s2_offset;

    uint8_t hdr_type = DSTRGETTYPE(s1);
    void* hd = NULL;
    switch (hdr_type) {
        case DSTRHD_TYPE_8:
            hd = DSTRGETHDR(8, s1);
            break;
        case DSTRHD_TYPE_16:
            hd = DSTRGETHDR(16, s1);
            break;
        case DSTRHD_TYPE_32:
            hd = DSTRGETHDR(32, s1);
            break;
        case DSTRHD_TYPE_64:
            hd = DSTRGETHDR(64, s1);
            break;
        default:
            return NULL;
    }
    memcpy(s1 + s1_len, s2, s2_len);
    s1[new_len] = '\0';
    _dstr_set_len(hd, new_len, hdr_type);
    return s1;
}

// strcat
dstr dstrcat_base(dstr s1, const char* s2)
{
    if (!s1 || !s2)
        return s1;
    size_t s1_len = dstrlen(s1);
    size_t s2_len = strlen(s2);
    size_t cap = _dstr_grow_cap(dstrcap(s1), s1_len + s2_len + 1);
    return _dstr_concat_impl(s1, s1_len, s2, s2_len, cap);
}

dstr dstrcat_custom(dstr s1, const char* s2, size_t cap)
{
    if (!s1 || !s2)
        return s1;
    return _dstr_concat_impl(s1, dstrlen(s1), s2, strlen(s2), cap);
}

// strpush
dstr dstrpush_custom(dstr s1, const char c, size_t cap)
{
    if (cap == 0 || !s1)
        return s1;

    size_t s1_len = dstrlen(s1);
    size_t s1_cap = dstrcap(s1);

    if (cap <= s1_cap)
        cap = s1_cap + 1;

    if (s1_cap <= s1_len + 1) {
        s1 = dstrreserve(s1, cap);
        if (!s1)
            return NULL;
    }

    enum dstrhd_type s1_type;
    void* s1_hd = _dstr_get_hdr_and_type(s1, &s1_type);

    s1[s1_len] = c;
    s1[s1_len + 1] = '\0';
    _dstr_set_len(s1_hd, s1_len + 1, s1_type);

    return s1;
}

dstr dstrpush_base(dstr s1, const char c)
{
    size_t cap = dstrcap(s1);
    return dstrpush_custom(s1, c, _dstr_grow_cap(cap, cap + 1));
}

// strappend
dstr dstrappend_base(dstr s1, const dstr s2)
{
    if (!s1 || !s2)
        return s1;
    size_t s1_len = dstrlen(s1);
    size_t s2_len = dstrlen(s2);
    size_t cap = _dstr_grow_cap(dstrcap(s1), s1_len + s2_len + 1);
    return _dstr_concat_impl(s1, s1_len, s2, s2_len, cap);
}

dstr dstrappend_custom(dstr s1, const dstr s2, size_t cap)
{
    if (!s1 || !s2)
        return s1;
    return _dstr_concat_impl(s1, dstrlen(s1), s2, dstrlen(s2), cap);
}

// dstrinsert
dstr dstrinsert(dstr s1, ssize_t idx, const char* s2)
{
    if (!s1 || !s2)
        return s1;

    size_t s1_len = dstrlen(s1);
    size_t s2_len = strlen(s2);
    if (s2_len == 0)
        return s1;
    if (s2_len > SIZE_MAX - s1_len)
        return NULL;

    if (idx < 0) {
        idx += (ssize_t)s1_len;
        if (idx < 0)
            idx = 0;
    }
    size_t pos = ((size_t)idx > s1_len) ? s1_len : (size_t)idx;
    size_t new_len = s1_len + s2_len;

    uintptr_t s1_addr = (uintptr_t)s1;
    uintptr_t s2_addr = (uintptr_t)s2;
    size_t s1_cap = dstrcap(s1);
    bool s2_aliases_s1 = s2_addr >= s1_addr && s2_addr - s1_addr < s1_cap;
    size_t s2_offset = s2_aliases_s1 ? (size_t)(s2_addr - s1_addr) : 0;

    dstr tmp = dstrreserve(s1, _dstr_grow_cap(s1_cap, new_len + 1));
    if (!tmp)
        return NULL;
    s1 = tmp;

    if (s2_aliases_s1)
        s2 = s1 + s2_offset;

    enum dstrhd_type t;
    void* hd = _dstr_get_hdr_and_type(s1, &t);

    size_t tail_len = s1_len - pos + 1;

    if (s2_aliases_s1 && s2_offset + s2_len > pos) {
        char* scratch = DSTR_MALLOC(s2_len);
        if (!scratch)
            return NULL;
        memcpy(scratch, s2, s2_len);
        memmove(s1 + pos + s2_len, s1 + pos, tail_len);
        memcpy(s1 + pos, scratch, s2_len);
        DSTR_FREE(scratch);
    } else {
        memmove(s1 + pos + s2_len, s1 + pos, tail_len);
        memcpy(s1 + pos, s2, s2_len);
    }

    _dstr_set_len(hd, new_len, t);
    return s1;
}

// strnew
static dstr _dstrnew_allocator(enum dstrhd_type t,
                               const char* msg,
                               size_t s_len,
                               size_t alloc_size)
{
    switch (t) {
        case DSTRHD_TYPE_8: {
            struct dstrhd8* hd = DSTR_MALLOC(sizeof(*hd) + alloc_size);
            if (!hd)
                return NULL;

            hd->cap = (uint8_t)alloc_size;
            hd->len = (uint8_t)s_len;
            hd->type = DSTRHD_TYPE_8;

            memcpy(hd->buf, msg, s_len + 1);
            return (dstr)hd->buf;
        }

        case DSTRHD_TYPE_16: {
            struct dstrhd16* hd = DSTR_MALLOC(sizeof(*hd) + alloc_size);
            if (!hd)
                return NULL;

            hd->cap = (uint16_t)alloc_size;
            hd->len = (uint16_t)s_len;
            hd->type = DSTRHD_TYPE_16;

            memcpy(hd->buf, msg, s_len + 1);
            return (dstr)hd->buf;
        }

        case DSTRHD_TYPE_32: {
            struct dstrhd32* hd = DSTR_MALLOC(sizeof(*hd) + alloc_size);
            if (!hd)
                return NULL;

            hd->cap = (uint32_t)alloc_size;
            hd->len = (uint32_t)s_len;
            hd->type = DSTRHD_TYPE_32;

            memcpy(hd->buf, msg, s_len + 1);
            return (dstr)hd->buf;
        }

        case DSTRHD_TYPE_64: {
            struct dstrhd64* hd = DSTR_MALLOC(sizeof(*hd) + alloc_size);
            if (!hd)
                return NULL;

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
    if (!msg)
        return NULL;
    size_t s_len = strlen(msg);
    size_t alloc_size = s_len + 1;

    enum dstrhd_type t = _dstr_type_by_size(alloc_size);

    return _dstrnew_allocator(t, msg, s_len, alloc_size);
}

dstr dstrnew_custom(const char* msg, size_t cap)
{
    if (!msg)
        return NULL;
    size_t s_len = strlen(msg);
    size_t alloc_size = (s_len + 1 > cap) ? (s_len + 1) : cap;

    enum dstrhd_type t = _dstr_type_by_size(alloc_size);

    return _dstrnew_allocator(t, msg, s_len, alloc_size);
}

// strfree
void dstrfree(dstr s)
{
    if (!s)
        return;
    enum dstrhd_type t = DSTRGETTYPE(s);

    switch (t) {
        case DSTRHD_TYPE_8: {
            DSTR_FREE(DSTRGETHDR(8, s));
            return;
        }
        case DSTRHD_TYPE_16: {
            DSTR_FREE(DSTRGETHDR(16, s));
            return;
        }
        case DSTRHD_TYPE_32: {
            DSTR_FREE(DSTRGETHDR(32, s));
            return;
        }
        case DSTRHD_TYPE_64: {
            DSTR_FREE(DSTRGETHDR(64, s));
            return;
        }
    }
}
