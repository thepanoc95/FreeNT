/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         FreeNT FreeDLL
 * FILE:            freedll/src/heap.c
 * PURPOSE:         Tiny C Runtime - heap management
 * PROGRAMMER:      FreeNT Team
 */

#include "freedll.h"

/* Simple heap manager using NT VirtualAlloc */

#define HEAP_SIGNATURE          0x46484541 /* "AEHF" */
#define HEAP_BLOCK_SIGNATURE    0x424C4542 /* "BELB" */
#define HEAP_DEFAULT_SIZE       (64 * 1024) /* 64KB initial heap */
#define HEAP_CHUNK_INUSE        0x10000000
#define HEAP_CHUNK_FREE         0x20000000
#define HEAP_BLOCK_SIZE         16  /* Minimum block size for alignment */
#define ALIGN_UP(x, a)          (((x) + (a) - 1) & ~((a) - 1))

typedef struct _HEAP_HEADER {
    ULONG Signature;
    ULONG Flags;
    SIZE_T TotalSize;
    struct _HEAP_HEADER *NextHeap;
    struct _HEAP_CHUNK_HEADER *FirstChunk;
    /* Lock could go here */
} HEAP_HEADER, *PHEAP_HEADER;

typedef struct _HEAP_CHUNK_HEADER {
    ULONG Signature;
    ULONG Flags;
    SIZE_T Size;
    struct _HEAP_CHUNK_HEADER *Next;
    struct _HEAP_CHUNK_HEADER *Prev;
} HEAP_CHUNK_HEADER, *PHEAP_CHUNK_HEADER;

/* The default process heap */
static HEAP_HEADER *g_ProcessHeap = NULL;
static HEAP_HEADER *g_HeapList = NULL;

/* System virtual memory allocation function (would call NtAllocateVirtualMemory) */
extern PVOID __freent_virtual_alloc(SIZE_T size);
extern VOID  __freent_virtual_free(PVOID addr);

/* Initialize the default process heap */
BOOLEAN InitializeProcessHeap(VOID)
{
    PVOID heap_mem;

    if (g_ProcessHeap != NULL)
        return TRUE;

    /* Allocate initial heap memory directly from the system via ntdll */
    heap_mem = __freent_virtual_alloc(HEAP_DEFAULT_SIZE);
    if (heap_mem == NULL)
        return FALSE;

    g_ProcessHeap = (HEAP_HEADER *)heap_mem;
    g_ProcessHeap->Signature = HEAP_SIGNATURE;
    g_ProcessHeap->Flags = 0;
    g_ProcessHeap->TotalSize = HEAP_DEFAULT_SIZE;
    g_ProcessHeap->NextHeap = NULL;
    g_ProcessHeap->FirstChunk = NULL;

    /* Create the first free chunk */
    PHEAP_CHUNK_HEADER chunk = (PHEAP_CHUNK_HEADER)(
        (UCHAR *)heap_mem + sizeof(HEAP_HEADER));
    chunk->Signature = HEAP_BLOCK_SIGNATURE;
    chunk->Flags = HEAP_CHUNK_FREE;
    chunk->Size = HEAP_DEFAULT_SIZE - sizeof(HEAP_HEADER) - sizeof(HEAP_CHUNK_HEADER);
    chunk->Next = NULL;
    chunk->Prev = NULL;

    g_ProcessHeap->FirstChunk = chunk;

    g_HeapList = g_ProcessHeap;
    return TRUE;
}

HANDLE GetProcessHeap(VOID)
{
    if (!InitializeProcessHeap())
        return NULL;
    return (HANDLE)g_ProcessHeap;
}

HANDLE CreateHeap(ULONG flags, SIZE_T reserve_size, SIZE_T commit_size)
{
    HANDLE heap;
    SIZE_T size = (reserve_size > 0) ? reserve_size : HEAP_DEFAULT_SIZE;

    /* Allocate heap memory directly from the system */
    heap = __freent_virtual_alloc(size);
    if (heap == NULL)
        return NULL;

    PHEAP_HEADER hdr = (PHEAP_HEADER)heap;
    hdr->Signature = HEAP_SIGNATURE;
    hdr->Flags = flags;
    hdr->TotalSize = size;
    hdr->NextHeap = g_HeapList;
    hdr->FirstChunk = NULL;

    SIZE_T chunk_size = size - sizeof(HEAP_HEADER) - sizeof(HEAP_CHUNK_HEADER);
    if (chunk_size > 0) {
        PHEAP_CHUNK_HEADER chunk = (PHEAP_CHUNK_HEADER)(
            (UCHAR *)heap + sizeof(HEAP_HEADER));
        chunk->Signature = HEAP_BLOCK_SIGNATURE;
        chunk->Flags = HEAP_CHUNK_FREE;
        chunk->Size = chunk_size;
        chunk->Next = NULL;
        chunk->Prev = NULL;
        hdr->FirstChunk = chunk;
    }

    g_HeapList = hdr;
    return (HANDLE)hdr;
}

BOOLEAN DestroyHeap(HANDLE heap)
{
    PHEAP_HEADER hdr = (PHEAP_HEADER)heap;
    if (hdr == NULL || hdr->Signature != HEAP_SIGNATURE)
        return FALSE;

    /* Remove from heap list */
    if (g_HeapList == hdr)
        g_HeapList = hdr->NextHeap;
    else {
        HEAP_HEADER *prev = g_HeapList;
        while (prev != NULL && prev->NextHeap != hdr)
            prev = prev->NextHeap;
        if (prev != NULL)
            prev->NextHeap = hdr->NextHeap;
    }

    /* Free the heap memory back to the system via virtual free */
    __freent_virtual_free((PVOID)hdr);
    return TRUE;
}

HANDLE RtlCreateHeap(ULONG flags, PVOID heap_base, SIZE_T reserve_size, SIZE_T commit_size, PVOID pslock, PRTL_HEAP_PARAMETERS parameters)
{
    return CreateHeap(flags, reserve_size, commit_size);
}

BOOLEAN RtlDestroyHeap(HANDLE heap)
{
    return DestroyHeap(heap);
}

/* Allocate from a heap */
PVOID RtlAllocateHeap(HANDLE heap, ULONG flags, SIZE_T size)
{
    PHEAP_HEADER hdr;
    PHEAP_CHUNK_HEADER chunk;
    size_t alloc_size;

    if (size == 0)
        return NULL;

    /* Align size to pointer boundary */
    alloc_size = ALIGN_UP(size, sizeof(ULONG_PTR));

    /* Get heap */
    if (heap == NULL)
        hdr = g_ProcessHeap;
    else
        hdr = (PHEAP_HEADER)heap;

    if (hdr == NULL) {
        if (!InitializeProcessHeap())
            return NULL;
        hdr = g_ProcessHeap;
    }

    if (hdr->Signature != HEAP_SIGNATURE)
        return NULL;

    /* Search for a free chunk that fits */
    for (chunk = hdr->FirstChunk; chunk != NULL; chunk = chunk->Next) {
        if ((chunk->Flags & HEAP_CHUNK_FREE) &&
            chunk->Size >= alloc_size + sizeof(HEAP_CHUNK_HEADER)) {

            /* Split chunk if large enough */
            if (chunk->Size >= alloc_size + sizeof(HEAP_CHUNK_HEADER) + sizeof(HEAP_CHUNK_HEADER)) {
                PHEAP_CHUNK_HEADER new_chunk;
                SIZE_T remaining;

                remaining = chunk->Size - alloc_size - sizeof(HEAP_CHUNK_HEADER);

                new_chunk = (PHEAP_CHUNK_HEADER)(
                    (UCHAR *)chunk + sizeof(HEAP_CHUNK_HEADER) + alloc_size);
                new_chunk->Signature = HEAP_BLOCK_SIGNATURE;
                new_chunk->Flags = HEAP_CHUNK_FREE;
                new_chunk->Size = remaining;
                new_chunk->Next = chunk->Next;
                new_chunk->Prev = chunk;

                if (chunk->Next != NULL)
                    chunk->Next->Prev = new_chunk;
                chunk->Next = new_chunk;

                chunk->Size = alloc_size;
            }

            chunk->Flags = HEAP_CHUNK_INUSE;
            return (PVOID)((UCHAR *)chunk + sizeof(HEAP_CHUNK_HEADER));
        }
    }

    /* No suitable chunk found */
    return NULL;
}

BOOLEAN RtlFreeHeap(HANDLE heap, ULONG flags, PVOID ptr)
{
    PHEAP_HEADER hdr;
    PHEAP_CHUNK_HEADER chunk;

    if (ptr == NULL)
        return FALSE;

    if (heap == NULL)
        hdr = g_ProcessHeap;
    else
        hdr = (PHEAP_HEADER)heap;

    if (hdr == NULL || hdr->Signature != HEAP_SIGNATURE)
        return FALSE;

    /* Get chunk header */
    chunk = (PHEAP_CHUNK_HEADER)((UCHAR *)ptr - sizeof(HEAP_CHUNK_HEADER));
    if (chunk->Signature != HEAP_BLOCK_SIGNATURE)
        return FALSE;

    if (!(chunk->Flags & HEAP_CHUNK_INUSE))
        return FALSE;

    /* Mark as free */
    chunk->Flags = HEAP_CHUNK_FREE;

    /* Coalesce with next chunk if free */
    if (chunk->Next != NULL && (chunk->Next->Flags & HEAP_CHUNK_FREE)) {
        PHEAP_CHUNK_HEADER next = chunk->Next;
        chunk->Size += sizeof(HEAP_CHUNK_HEADER) + next->Size;
        chunk->Next = next->Next;
        if (next->Next != NULL)
            next->Next->Prev = chunk;
    }

    /* Coalesce with previous chunk if free */
    if (chunk->Prev != NULL && (chunk->Prev->Flags & HEAP_CHUNK_FREE)) {
        PHEAP_CHUNK_HEADER prev = chunk->Prev;
        prev->Size += sizeof(HEAP_CHUNK_HEADER) + chunk->Size;
        prev->Next = chunk->Next;
        if (chunk->Next != NULL)
            chunk->Next->Prev = prev;
    }

    return TRUE;
}

PVOID RtlReAllocateHeap(HANDLE heap, ULONG flags, PVOID ptr, SIZE_T size)
{
    PHEAP_CHUNK_HEADER chunk;
    PVOID new_ptr;
    SIZE_T old_size;

    if (ptr == NULL)
        return RtlAllocateHeap(heap, flags, size);

    if (size == 0) {
        RtlFreeHeap(heap, flags, ptr);
        return NULL;
    }

    /* Get current chunk */
    chunk = (PHEAP_CHUNK_HEADER)((UCHAR *)ptr - sizeof(HEAP_CHUNK_HEADER));
    old_size = chunk->Size;

    /* If new size fits in current chunk, just return */
    if (size <= old_size)
        return ptr;

    /* Allocate new block and copy */
    new_ptr = RtlAllocateHeap(heap, flags, size);
    if (new_ptr == NULL)
        return NULL;

    freent_memcpy(new_ptr, ptr, old_size);
    RtlFreeHeap(heap, flags, ptr);

    return new_ptr;
}

SIZE_T RtlSizeHeap(HANDLE heap, ULONG flags, PVOID ptr)
{
    PHEAP_CHUNK_HEADER chunk;

    if (ptr == NULL)
        return 0;

    chunk = (PHEAP_CHUNK_HEADER)((UCHAR *)ptr - sizeof(HEAP_CHUNK_HEADER));
    if (chunk->Signature != HEAP_BLOCK_SIGNATURE)
        return 0;

    return chunk->Size;
}
