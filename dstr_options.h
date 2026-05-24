#ifndef DSTR_OPTIONS
#define DSTR_OPTIONS

/*
 * This option will be used in dstr.c
 */

#define DSTR_MALLOC(s)      malloc(s)
#define DSTR_REALLOC(p, s)  realloc(p, s)
#define DSTR_FREE(s)        free(s)

#endif
