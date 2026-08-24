/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         FreeNT FreeDLL
 * FILE:            freedll/src/crt_compat.c
 * PURPOSE:         CRT compatibility wrappers - maps standard names to freent_*
 * PROGRAMMER:      FreeNT Team
 */

#include "freedll.h"

/*
 * This file provides forwarders from standard CRT function names
 * to the freent_* implementations. This allows applications compiled
 * against the MSVC/CRT ABI to import these functions seamlessly.
 */

/* Memory functions */
void *memcpy(void *dest, const void *src, size_t n)
{
    return freent_memcpy(dest, src, n);
}

void *memmove(void *dest, const void *src, size_t n)
{
    return freent_memmove(dest, src, n);
}

void *memset(void *s, int c, size_t n)
{
    return freent_memset(s, c, n);
}

int memcmp(const void *s1, const void *s2, size_t n)
{
    return freent_memcmp(s1, s2, n);
}

void *memchr(const void *s, int c, size_t n)
{
    return freent_memchr(s, c, n);
}

/* String functions */
size_t strlen(const char *s)
{
    return freent_strlen(s);
}

size_t strnlen(const char *s, size_t maxlen)
{
    return freent_strnlen(s, maxlen);
}

char *strcpy(char *dest, const char *src)
{
    return freent_strcpy(dest, src);
}

char *strncpy(char *dest, const char *src, size_t n)
{
    return freent_strncpy(dest, src, n);
}

char *strcat(char *dest, const char *src)
{
    return freent_strcat(dest, src);
}

char *strncat(char *dest, const char *src, size_t n)
{
    return freent_strncat(dest, src, n);
}

int strcmp(const char *s1, const char *s2)
{
    return freent_strcmp(s1, s2);
}

int strncmp(const char *s1, const char *s2, size_t n)
{
    return freent_strncmp(s1, s2, n);
}

char *strchr(const char *s, int c)
{
    return freent_strchr(s, c);
}

char *strrchr(const char *s, int c)
{
    return freent_strrchr(s, c);
}

char *strstr(const char *haystack, const char *needle)
{
    return freent_strstr(haystack, needle);
}

char *strtok(char *s, const char *delim)
{
    return freent_strtok(s, delim);
}

char *strdup(const char *s)
{
    return freent_strdup(s);
}

/* Wide string functions */
size_t wcslen(const WCHAR *s)
{
    return freent_wcslen(s);
}

int wcscmp(const WCHAR *s1, const WCHAR *s2)
{
    return freent_wcscmp(s1, s2);
}

int wcsncmp(const WCHAR *s1, const WCHAR *s2, size_t n)
{
    return freent_wcsncmp(s1, s2, n);
}

WCHAR *wcscpy(WCHAR *dest, const WCHAR *src)
{
    return freent_wcscpy(dest, src);
}

WCHAR *wcsncpy(WCHAR *dest, const WCHAR *src, size_t n)
{
    return freent_wcsncpy(dest, src, n);
}

/* Standard library */
int atoi(const char *nptr)
{
    return freent_atoi(nptr);
}

long atol(const char *nptr)
{
    return freent_atol(nptr);
}

long long atoll(const char *nptr)
{
    return freent_atoll(nptr);
}

int abs(int j)
{
    return freent_abs(j);
}

long labs(long j)
{
    return freent_labs(j);
}

void qsort(void *base, size_t nmemb, size_t size,
           int (*compar)(const void *, const void *))
{
    freent_qsort(base, nmemb, size, compar);
}

/* Format functions */
int snprintf(char *buffer, size_t size, const char *format, ...)
{
    int result;
    va_list ap;
    va_start(ap, format);
    result = freent_vsnprintf(buffer, size, format, ap);
    va_end(ap);
    return result;
}

int vsnprintf(char *buffer, size_t size, const char *format, va_list ap)
{
    return freent_vsnprintf(buffer, size, format, ap);
}
