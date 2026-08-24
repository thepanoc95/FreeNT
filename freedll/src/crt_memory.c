/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         FreeNT FreeDLL
 * FILE:            freedll/src/crt_memory.c
 * PURPOSE:         Tiny C Runtime - memory functions
 * PROGRAMMER:      FreeNT Team
 */

#include "freedll.h"

void *freent_memcpy(void *dest, const void *src, size_t n)
{
    unsigned char *d = (unsigned char *)dest;
    const unsigned char *s = (const unsigned char *)src;
    size_t i;

    if (d == s)
        return dest;

    /* Optimize with word-sized copies when possible */
    if (n >= sizeof(ULONG_PTR) && ((size_t)d % sizeof(ULONG_PTR)) == 0 &&
        ((size_t)s % sizeof(ULONG_PTR)) == 0) {
        ULONG_PTR *wd = (ULONG_PTR *)d;
        const ULONG_PTR *ws = (const ULONG_PTR *)s;
        size_t words = n / sizeof(ULONG_PTR);
        for (i = 0; i < words; i++)
            wd[i] = ws[i];
        d = (unsigned char *)(wd + words);
        s = (const unsigned char *)(ws + words);
        n -= words * sizeof(ULONG_PTR);
    }

    for (i = 0; i < n; i++)
        d[i] = s[i];

    return dest;
}

void *freent_memmove(void *dest, const void *src, size_t n)
{
    unsigned char *d = (unsigned char *)dest;
    const unsigned char *s = (const unsigned char *)src;
    size_t i;

    if (d == s)
        return dest;

    if (d < s) {
        for (i = 0; i < n; i++)
            d[i] = s[i];
    } else {
        for (i = n; i > 0; i--)
            d[i - 1] = s[i - 1];
    }

    return dest;
}

void *freent_memset(void *s, int c, size_t n)
{
    unsigned char *p = (unsigned char *)s;
    size_t i;
    unsigned char fill = (unsigned char)c;

    /* Optimize with pattern fill when possible */
    if (n >= sizeof(ULONG_PTR) && ((size_t)p % sizeof(ULONG_PTR)) == 0) {
        ULONG_PTR pattern = 0;
        for (i = 0; i < sizeof(ULONG_PTR); i++)
            ((unsigned char *)&pattern)[i] = fill;
        ULONG_PTR *wp = (ULONG_PTR *)p;
        size_t words = n / sizeof(ULONG_PTR);
        for (i = 0; i < words; i++)
            wp[i] = pattern;
        p = (unsigned char *)(wp + words);
        n -= words * sizeof(ULONG_PTR);
    }

    for (i = 0; i < n; i++)
        p[i] = fill;

    return s;
}

int freent_memcmp(const void *s1, const void *s2, size_t n)
{
    const unsigned char *p1 = (const unsigned char *)s1;
    const unsigned char *p2 = (const unsigned char *)s2;
    size_t i;

    for (i = 0; i < n; i++) {
        if (p1[i] != p2[i])
            return p1[i] - p2[i];
    }
    return 0;
}

void *freent_memchr(const void *s, int c, size_t n)
{
    const unsigned char *p = (const unsigned char *)s;
    unsigned char target = (unsigned char)c;
    size_t i;

    for (i = 0; i < n; i++) {
        if (p[i] == target)
            return (void *)&p[i];
    }
    return NULL;
}
