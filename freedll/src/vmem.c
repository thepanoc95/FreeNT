/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         FreeNT FreeDLL
 * FILE:            freedll/src/vmem.c
 * PURPOSE:         Virtual memory allocation interface to ntdll
 * PROGRAMMER:      FreeNT Team
 */

#include "freedll.h"

/*
 * This file provides virtual memory allocation by delegating to ntdll's
 * NtAllocateVirtualMemory/NtFreeVirtualMemory syscalls.
 */

/* Memory protection and allocation type constants */
#define MEM_COMMIT      0x00001000
#define MEM_RESERVE     0x00002000
#define MEM_RELEASE     0x00008000
#define MEM_FREE        0x00010000
#define PAGE_READWRITE  0x04

/* System allocation helper - used by heap.c */
PVOID __freent_virtual_alloc(SIZE_T size)
{
    PVOID base_address = NULL;
    SIZE_T region_size = size;
    NTSTATUS status;

    /* Allocate executable-read-write memory from the system */
    status = NtAllocateVirtualMemory(
        (HANDLE)(ULONG_PTR)-1,           /* Current process */
        &base_address,
        NULL,                             /* ZeroBits */
        &region_size,
        MEM_COMMIT | MEM_RESERVE,
        PAGE_READWRITE
    );

    if (!NT_SUCCESS(status) || base_address == NULL)
        return NULL;

    return base_address;
}

/* System free helper - used by heap.c */
VOID __freent_virtual_free(PVOID addr)
{
    PVOID base_address = addr;
    SIZE_T region_size = 0;

    NtFreeVirtualMemory(
        (HANDLE)(ULONG_PTR)-1,           /* Current process */
        &base_address,
        &region_size,
        MEM_RELEASE
    );
}
