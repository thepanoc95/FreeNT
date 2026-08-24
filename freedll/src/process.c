/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         FreeNT FreeDLL
 * FILE:            freedll/src/process.c
 * PURPOSE:         Process and thread management functions
 * PROGRAMMER:      FreeNT Team
 */

#include "freedll.h"

/* Process information structure */
typedef struct _PROCESS_BASIC_INFORMATION {
    NTSTATUS ExitStatus;
    PVOID    PebBaseAddress;
    ULONG_PTR Reserved0;
    ULONG_PTR UniqueProcessId;
    ULONG_PTR Reserved1;
    ULONG_PTR inherited_from_unique_process_id;
} PROCESS_BASIC_INFORMATION, *PPROCESS_BASIC_INFORMATION;

typedef LONG KPRIORITY;

typedef struct _PROCESS_EXTENDED_BASIC_INFORMATION {
    ULONG Size;
    ULONG BreakOnTerminationFlags;
    ULONG ProcessFlags;
    HANDLE UniqueProcessId;
    LONG   PriorityClass;
} PROCESS_EXTENDED_BASIC_INFORMATION, *PPROCESS_EXTENDED_BASIC_INFORMATION;

/* Current process and thread identifiers */
/* On x64 Windows:
 * - gs:0x30 = TEB self-pointer
 * - gs:0x40 = PEB pointer
 * ProcessId and ThreadId are not directly in the TEB; we use system calls.
 */

HANDLE GetCurrentProcess(VOID)
{
    /* GetCurrentProcess returns a pseudo-handle (-1) */
    return (HANDLE)(LONG_PTR)-1;
}

HANDLE GetCurrentThread(VOID)
{
    /* GetCurrentThread returns a pseudo-handle (-2) */
    return (HANDLE)(LONG_PTR)-2;
}

DWORD GetCurrentProcessId(VOID)
{
    /* In a real implementation, this would use NtQueryInformationProcess */
    /* For now, read from PEB which contains process info */
    PPEB_SUBSET peb;
    __asm__ volatile("movq %%gs:0x60, %0" : "=r"(peb));
    /* The inherited_from_unique_process_id is in PEB, but for the current
       process, we'd need a syscall. Return a placeholder. */
    /* In practice, this would call NtQueryInformationProcess */
    return 0;
}

DWORD GetCurrentThreadId(VOID)
{
    /* In a real implementation, this would use NtQueryInformationThread */
    /* Return a placeholder - would call NtQueryInformationThread */
    return 0;
}

/* ExitProcess - terminates the current process */
VOID ExitProcess(DWORD ExitCode)
{
    /* Call NtTerminateProcess with current process handle */
    NtTerminateProcess((HANDLE)(ULONG_PTR)-1, (NTSTATUS)ExitCode);

    /* Should never reach here */
    for (;;) { }
}

/* RtlExitUserProcess - exit the user process */
VOID RtlExitUserProcess(NTSTATUS Status)
{
    NtTerminateProcess((HANDLE)(ULONG_PTR)-1, Status);
    /* Should never reach here */
    for (;;) { }
}

/* GetTickCount - returns milliseconds since system start */
DWORD GetTickCount(VOID)
{
    /* In a real implementation, this would use NtQuerySystemTime.
       For now, return 0 as a minimal stub. */
    return 0;
}

/* QueryPerformanceCounter */
BOOLEAN QueryPerformanceCounter(PLARGE_INTEGER lpPerformanceCount)
{
    /* Would use NtQuerySystemTime in a real implementation */
    if (lpPerformanceCount)
        lpPerformanceCount->QuadPart = 0;
    return TRUE;
}

/* QueryPerformanceFrequency */
BOOLEAN QueryPerformanceFrequency(PLARGE_INTEGER lpFrequency)
{
    if (lpFrequency)
        lpFrequency->QuadPart = 10000000; /* 10 MHz - matches 100ns granularity */
    return TRUE;
}

/* GetProcessShutdownParameters */
BOOLEAN GetProcessShutdownParameters(LPDWORD lpdwFlags, HANDLE *lphand)
{
    if (lpdwFlags)
        *lpdwFlags = 0;
    if (lphand)
        *lphand = NULL;
    return TRUE;
}
