/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         FreeNT FreeDLL
 * FILE:            freedll/src/crt_string.c
 * PURPOSE:         Tiny C Runtime - string functions (ANSI)
 * PROGRAMMER:      FreeNT Team
 */

#include "freedll.h"

/* ===== String length ===== */

size_t freent_strlen(const char *s)
{
    size_t len = 0;
    while (s[len] != '\0')
        len++;
    return len;
}

size_t freent_strnlen(const char *s, size_t maxlen)
{
    size_t i;
    for (i = 0; i < maxlen; i++) {
        if (s[i] == '\0')
            break;
    }
    return i;
}

/* ===== String copy ===== */

char *freent_strcpy(char *dest, const char *src)
{
    size_t i = 0;
    while ((dest[i] = src[i]) != '\0')
        i++;
    return dest;
}

char *freent_strncpy(char *dest, const char *src, size_t n)
{
    size_t i;
    for (i = 0; i < n && src[i] != '\0'; i++)
        dest[i] = src[i];
    for (; i < n; i++)
        dest[i] = '\0';
    return dest;
}

/* ===== String concatenation ===== */

char *freent_strcat(char *dest, const char *src)
{
    size_t dest_len = freent_strlen(dest);
    size_t i = 0;
    while ((dest[dest_len + i] = src[i]) != '\0')
        i++;
    return dest;
}

char *freent_strncat(char *dest, const char *src, size_t n)
{
    size_t dest_len = freent_strlen(dest);
    size_t i;
    for (i = 0; i < n && src[i] != '\0'; i++)
        dest[dest_len + i] = src[i];
    dest[dest_len + i] = '\0';
    return dest;
}

/* ===== String comparison ===== */

int freent_strcmp(const char *s1, const char *s2)
{
    size_t i = 0;
    while (s1[i] != '\0' && s1[i] == s2[i])
        i++;
    return (int)((unsigned char)s1[i] - (unsigned char)s2[i]);
}

int freent_strncmp(const char *s1, const char *s2, size_t n)
{
    size_t i;
    if (n == 0)
        return 0;
    for (i = 0; i < n && s1[i] == s2[i]; i++) {
        if (i + 1 == n || s1[i] == '\0')
            break;
    }
    return (int)((unsigned char)s1[i] - (unsigned char)s2[i]);
}

/* ===== String search ===== */

char *freent_strchr(const char *s, int c)
{
    char target = (char)c;
    size_t i = 0;
    while (s[i] != '\0') {
        if (s[i] == target)
            return (char *)&s[i];
        i++;
    }
    if (s[i] == target)
        return (char *)&s[i];
    return NULL;
}

char *freent_strrchr(const char *s, int c)
{
    char target = (char)c;
    size_t i = freent_strlen(s);
    do {
        if (s[i] == target)
            return (char *)&s[i];
        if (i == 0)
            break;
        i--;
    } while (1);
    return NULL;
}

char *freent_strstr(const char *haystack, const char *needle)
{
    size_t h_len, n_len;
    size_t i;

    if (haystack == NULL || needle == NULL)
        return NULL;

    n_len = freent_strlen(needle);
    h_len = freent_strlen(haystack);

    if (n_len == 0)
        return (char *)haystack;

    if (n_len > h_len)
        return NULL;

    for (i = 0; i <= h_len - n_len; i++) {
        if (freent_strncmp(&haystack[i], needle, n_len) == 0)
            return (char *)&haystack[i];
    }
    return NULL;
}

char *freent_strtok(char *s, const char *delim)
{
    static char *next = NULL;
    char *start, *end;

    if (s != NULL)
        next = s;

    if (next == NULL || *next == '\0')
        return NULL;

    /* Skip leading delimiters */
    start = next;
    while (*start != '\0' && freent_strchr(delim, *start) != NULL)
        start++;

    if (*start == '\0') {
        next = NULL;
        return NULL;
    }

    /* Find end of token */
    end = start;
    while (*end != '\0' && freent_strchr(delim, *end) == NULL)
        end++;

    if (*end != '\0') {
        *end = '\0';
        next = end + 1;
    } else {
        next = NULL;
    }

    return start;
}

char *freent_strdup(const char *s)
{
    size_t len = freent_strlen(s) + 1;
    char *dup = (char *)RtlAllocateHeap(GetProcessHeap(), 0, len);
    if (dup != NULL)
        freent_memcpy(dup, s, len);
    return dup;
}

/* ===== Wide string functions ===== */

size_t freent_wcslen(const WCHAR *s)
{
    size_t len = 0;
    while (s[len] != L'\0')
        len++;
    return len;
}

int freent_wcscmp(const WCHAR *s1, const WCHAR *s2)
{
    size_t i = 0;
    while (s1[i] != L'\0' && s1[i] == s2[i])
        i++;
    return (int)(s1[i] - s2[i]);
}

int freent_wcsncmp(const WCHAR *s1, const WCHAR *s2, size_t n)
{
    size_t i;
    if (n == 0)
        return 0;
    for (i = 0; i < n && s1[i] == s2[i]; i++) {
        if (i + 1 == n || s1[i] == L'\0')
            break;
    }
    return (int)(s1[i] - s2[i]);
}

WCHAR *freent_wcscpy(WCHAR *dest, const WCHAR *src)
{
    size_t i = 0;
    while ((dest[i] = src[i]) != L'\0')
        i++;
    return dest;
}

WCHAR *freent_wcsncpy(WCHAR *dest, const WCHAR *src, size_t n)
{
    size_t i;
    for (i = 0; i < n && src[i] != L'\0'; i++)
        dest[i] = src[i];
    for (; i < n; i++)
        dest[i] = L'\0';
    return dest;
}

/* ===== Standard library ===== */

int freent_atoi(const char *nptr)
{
    int result = 0;
    int sign = 1;
    int i = 0;

    /* Skip whitespace */
    while (nptr[i] == ' ' || nptr[i] == '\t' || nptr[i] == '\n')
        i++;

    /* Check for sign */
    if (nptr[i] == '-') {
        sign = -1;
        i++;
    } else if (nptr[i] == '+') {
        i++;
    }

    /* Process digits */
    while (nptr[i] >= '0' && nptr[i] <= '9') {
        result = result * 10 + (nptr[i] - '0');
        i++;
    }

    return result * sign;
}

long freent_atol(const char *nptr)
{
    return (long)freent_atoi(nptr);
}

long long freent_atoll(const char *nptr)
{
    long long result = 0;
    int sign = 1;
    int i = 0;

    while (nptr[i] == ' ' || nptr[i] == '\t' || nptr[i] == '\n')
        i++;

    if (nptr[i] == '-') {
        sign = -1;
        i++;
    } else if (nptr[i] == '+') {
        i++;
    }

    while (nptr[i] >= '0' && nptr[i] <= '9') {
        result = result * 10 + (nptr[i] - '0');
        i++;
    }

    return result * sign;
}

int freent_abs(int j)
{
    return (j < 0) ? -j : j;
}

long freent_labs(long j)
{
    return (j < 0) ? -j : j;
}

void freent_qsort(void *base, size_t nmemb, size_t size,
                  int (*compar)(const void *, const void *))
{
    unsigned char *arr = (unsigned char *)base;
    unsigned char *stack[64];
    int stack_ptr = 0;
    size_t i, j;
    size_t pivot;
    unsigned char *tmp;
    unsigned char *swap1, *swap2;
    size_t tmp_size;

    if (nmemb < 2 || size == 0 || compar == NULL)
        return;

    /* Simple iterative quicksort */
    stack[0] = arr;
    stack[1] = arr + (nmemb - 1) * size;

    while (stack_ptr < 2) {
        unsigned char *lo = stack[stack_ptr];
        unsigned char *hi = stack[stack_ptr + 1];
        stack_ptr += 2;

        while (lo < hi) {
            /* Median-of-three pivot selection */
            unsigned char *mid = lo + ((hi - lo) / (long)(size * 2)) * size;
            if (compar(lo, mid) > 0) {
                tmp = lo; lo = mid; mid = tmp;
            }
            if (compar(lo, hi) > 0) {
                tmp = lo; lo = hi; hi = tmp;
            }
            if (compar(mid, hi) > 0) {
                tmp = mid; mid = hi; hi = tmp;
            }

            /* Partition */
            i = (size_t)((mid - lo) / size) + 1;
            j = (size_t)((hi - mid) / size);

            swap1 = mid + size;
            swap2 = hi;

            while (i <= j) {
                while (compar(lo + i * size, mid) < 0)
                    i++;
                while (compar(hi - j * size, mid) > 0)
                    j--;
                if (i > j)
                    break;

                if (lo + i * size != hi - j * size) {
                    tmp_size = size;
                    swap1 = lo + i * size;
                    swap2 = hi - j * size;
                    while (tmp_size--) {
                        unsigned char t = *swap1;
                        *swap1++ = *swap2;
                        *swap2++ = t;
                    }
                }
                i++;
                j--;
            }

            /* Push larger partition */
            if (j > (size_t)((mid - lo) / size)) {
                stack[stack_ptr++] = lo + j * size;
                stack[stack_ptr++] = hi;
            }

            /* Recurse on smaller partition */
            if ((size_t)((hi - mid) / size) > i) {
                stack[stack_ptr++] = mid + i * size;
                stack[stack_ptr++] = hi;
            }

            /* Use the smaller part, loop on the larger */
            if (i <= j) {
                if (i > (size_t)((hi - mid) / size)) {
                    lo = mid + i * size;
                }
            } else {
                lo = mid + size;
            }
        }
    }
}
