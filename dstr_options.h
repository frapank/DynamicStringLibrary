#ifndef DSTR_OPTIONS
#define DSTR_OPTIONS

/*
 * This option will be used in dstr.c
 */

#define DSTR_MALLOC(s)      malloc(s)
#define DSTR_REALLOC(p, s)  realloc(p, s)
#define DSTR_FREE(s)        free(s)

/*
 * Amortized growth for the dstrcat/dstrappend/dstrpush base variants.
 * 1 = grow ahead of need to reduce future reallocations
 * 0 = allocate exactly what is needed, nothing more
 * The custom variants (and dstrnew's cap argument) always honor the
 * caller-supplied capacity regardless of this setting.
 */
#define DSTR_GROWTH_ENABLED 1

/*
 * Extra capacity requested on growth, as a percentage of the current
 * capacity. Only used when DSTR_GROWTH_ENABLED is 1. Must be > 0.
 * 50 means +50% (new capacity is roughly 1.5x the old one).
 */
#define DSTR_GROWTH_PERCENT 50

#endif
