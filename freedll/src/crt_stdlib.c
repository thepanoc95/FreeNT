/* ISO C utility functions implemented without a host CRT. */
#include "freedll.h"

static unsigned long freent_rand_state = 1;

void *freent_malloc(size_t size)
{
    return size ? RtlAllocateHeap(GetProcessHeap(), 0, size) : NULL;
}

void freent_free(void *ptr)
{
    if (ptr) RtlFreeHeap(GetProcessHeap(), 0, ptr);
}

void *freent_calloc(size_t count, size_t size)
{
    size_t total;
    void *ptr;
    if (size && count > (size_t)-1 / size) return NULL;
    total = count * size;
    ptr = freent_malloc(total);
    if (ptr) freent_memset(ptr, 0, total);
    return ptr;
}

void *freent_realloc(void *ptr, size_t size)
{
    if (!ptr) return freent_malloc(size);
    if (!size) { freent_free(ptr); return NULL; }
    return RtlReAllocateHeap(GetProcessHeap(), 0, ptr, size);
}

size_t freent_strspn(const char *s, const char *accept)
{
    size_t n = 0;
    if (!s || !accept) return 0;
    while (s[n] && freent_strchr(accept, s[n])) ++n;
    return n;
}

size_t freent_strcspn(const char *s, const char *reject)
{
    size_t n = 0;
    if (!s || !reject) return 0;
    while (s[n] && !freent_strchr(reject, s[n])) ++n;
    return n;
}

char *freent_strpbrk(const char *s, const char *accept)
{
    if (!s || !accept) return NULL;
    while (*s) { if (freent_strchr(accept, *s)) return (char *)s; ++s; }
    return NULL;
}

void *freent_bsearch(const void *key, const void *base, size_t count, size_t size,
                     int (*compar)(const void *, const void *))
{
    size_t low = 0, high = count;
    if (!key || !base || !size || !compar) return NULL;
    while (low < high) {
        size_t mid = low + (high - low) / 2;
        const void *element = (const UCHAR *)base + mid * size;
        int order = compar(key, element);
        if (!order) return (void *)element;
        if (order < 0) high = mid; else low = mid + 1;
    }
    return NULL;
}

int freent_rand(void)
{
    freent_rand_state = freent_rand_state * 1103515245UL + 12345UL;
    return (int)((freent_rand_state >> 16) & 0x7fffU);
}

void freent_srand(unsigned int seed) { freent_rand_state = seed; }
