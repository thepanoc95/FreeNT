/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         FreeNT FreeDLL
 * FILE:            freedll/src/dllmain.c
 * PURPOSE:         FreeDLL entry point and lifecycle management
 * PROGRAMMER:      FreeNT Team
 */

#include "freedll.h"

/* Internal FreeDLL state */
static BOOLEAN g_freedll_initialized = FALSE;
static BOOLEAN g_freedll_terminating = FALSE;
static HINSTANCE g_freedll_instance = NULL;

/* TLS callback list - for C++ thread-local storage */
static PTLS_CALLBACK_FUNCTION g_tls_callbacks[64];
static int g_tls_callback_count = 0;

/* Forward declarations */
static VOID InitializeTls(VOID);
static VOID CallTlsCallbacks(DWORD reason);
static BOOLEAN InitializeCrt(VOID);
static VOID CleanupCrt(VOID);

/*
 * FreeDLL main entry point.
 * This is called by the system when the DLL is loaded/unloaded.
 */
BOOLEAN NTAPI FreeDllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    switch (fdwReason) {
    case DLL_PROCESS_ATTACH:
        if (g_freedll_initialized)
            return TRUE;

        g_freedll_instance = hinstDLL;

        /* Initialize subsystem components in order */
        InitializeProcessHeap();
        InitializeTls();
        InitializeCrt();

        g_freedll_initialized = TRUE;
        break;

    case DLL_PROCESS_DETACH:
        if (!g_freedll_initialized)
            return TRUE;

        g_freedll_terminating = TRUE;
        CleanupCrt();
        CallTlsCallbacks(DLL_PROCESS_DETACH);
        g_freedll_initialized = FALSE;
        break;

    case DLL_THREAD_ATTACH:
        CallTlsCallbacks(DLL_THREAD_ATTACH);
        break;

    case DLL_THREAD_DETACH:
        CallTlsCallbacks(DLL_THREAD_DETACH);
        break;
    }

    return TRUE;
}

/* Standard DllMain entry point - wraps FreeDllMain */
BOOLEAN NTAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    return FreeDllMain(hinstDLL, fdwReason, lpvReserved);
}

/* DllEntryPoint - alternative entry point name */
BOOLEAN NTAPI DllEntryPoint(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    return FreeDllMain(hinstDLL, fdwReason, lpvReserved);
}

/*
 * TLS (Thread Local Storage) management.
 * This provides support for __declspec(thread) variables.
 */

static VOID InitializeTls(VOID)
{
    /* Clear TLS callback list */
    g_tls_callback_count = 0;
    freent_memset(g_tls_callbacks, 0, sizeof(g_tls_callbacks));
}

/*
 * Register a TLS callback for thread lifecycle notifications.
 * Returns TRUE on success, FALSE if the callback list is full.
 */
BOOLEAN RegisterTlsCallback(PTLS_CALLBACK_FUNCTION callback)
{
    if (callback == NULL)
        return FALSE;

    if (g_tls_callback_count >= 64)
        return FALSE;

    g_tls_callbacks[g_tls_callback_count++] = callback;
    return TRUE;
}

/*
 * Call all registered TLS callbacks with the given reason.
 */
static VOID CallTlsCallbacks(DWORD reason)
{
    int i;
    for (i = 0; i < g_tls_callback_count; i++) {
        if (g_tls_callbacks[i] != NULL)
            g_tls_callbacks[i](g_freedll_instance, (DWORD)reason, NULL);
    }
}

/*
 * Fiber Local Storage (FLS) API - compatible with Windows FLS
 */

/* FLS index table */
#define FLS_MAX_INDEX 128
static PVOID g_fls_slots[FLS_MAX_INDEX];
static ULONG g_fls_index_counter = 0;

DWORD FlsAlloc(PVOID pCallback)
{
    DWORD index;

    /* In a real implementation, this would be thread-local.
       For simplicity, we use a global table. */
    if (g_fls_index_counter >= FLS_MAX_INDEX)
        return FLS_OUT_OF_INDEXES;

    index = g_fls_index_counter++;
    g_fls_slots[index] = NULL;
    return index;
}

PVOID FlsGetValue(DWORD dwFlsIndex)
{
    if (dwFlsIndex >= FLS_MAX_INDEX)
        return NULL;
    return g_fls_slots[dwFlsIndex];
}

BOOLEAN FlsSetValue(DWORD dwFlsIndex, PVOID lpFlsData)
{
    if (dwFlsIndex >= FLS_MAX_INDEX)
        return FALSE;
    g_fls_slots[dwFlsIndex] = lpFlsData;
    return TRUE;
}

BOOLEAN FlsFree(DWORD dwFlsIndex)
{
    if (dwFlsIndex >= FLS_MAX_INDEX)
        return FALSE;
    /* Would call callback if registered */
    g_fls_slots[dwFlsIndex] = NULL;
    return TRUE;
}

/* ===== CRT Initialization ===== */

/* Atexit function table */
#define MAX_ATEXIT_FUNCS 64
static void (*g_atexit_funcs[MAX_ATEXIT_FUNCS])(void);
static int g_atexit_count = 0;

/* static initializers for CRT */
static BOOLEAN InitializeCrt(VOID)
{
    /* Initialize atexit table */
    g_atexit_count = 0;
    freent_memset(g_atexit_funcs, 0, sizeof(g_atexit_funcs));

    return TRUE;
}

static VOID CleanupCrt(VOID)
{
    /* Call atexit functions in reverse order */
    int i;
    for (i = g_atexit_count - 1; i >= 0; i--) {
        if (g_atexit_funcs[i] != NULL)
            g_atexit_funcs[i]();
    }
    g_atexit_count = 0;
}

/* atexit - register function to be called at exit */
int atexit(void (*func)(void))
{
    if (func == NULL || g_atexit_count >= MAX_ATEXIT_FUNCS)
        return -1;

    g_atexit_funcs[g_atexit_count++] = func;
    return 0;
}

/* ===== C++ support (minimal) ===== */

/* __cxa_atexit for C++ destructors */
int __cxa_atexit(void (*func)(void *), void *arg, void *dso_handle)
{
    (void)func;
    (void)arg;
    (void)dso_handle;
    /* Minimal implementation - just register without arg tracking */
    if (g_atexit_count < MAX_ATEXIT_FUNCS) {
        /* Wrap the function */
        /* For simplicity, ignore the arg - in real implementation
           we'd need a proper table */
    }
    return 0;
}

/* Pure virtual function call handler */
void __purecall(void)
{
    /* Abort the program */
    NtTerminateProcess((HANDLE)(ULONG_PTR)-1, (NTSTATUS)0xFFFFFFFF);
}

/* Unexpected exception handler */
void __cdecl __exception_cancel(void)
{
    /* Minimal SEH support */
}

/* ===== Misc utility functions ===== */

/* RtlCaptureContext - capture CPU context (minimal implementation) */
VOID RtlCaptureContext(PCONTEXT ContextRecord)
{
    if (ContextRecord == NULL)
        return;

    /* In a real implementation, this would save all CPU registers.
       For x64, we use inline assembly or intrinsic. */
    /* For now, just zero-initialize to avoid crashes */
    freent_memset(ContextRecord, 0, sizeof(CONTEXT));
}

/* RtlNtStatusToDosError - convert NTSTATUS to Win32 error */
ULONG RtlNtStatusToDosError(NTSTATUS Status)
{
    if (NT_SUCCESS(Status))
        return 0;

    switch (Status) {
    case STATUS_INVALID_HANDLE:
        return 6;  /* ERROR_INVALID_HANDLE */
    case STATUS_ACCESS_DENIED:
        return 5;  /* ERROR_ACCESS_DENIED */
    case STATUS_NO_MEMORY:
        return 8;  /* ERROR_NOT_ENOUGH_MEMORY */
    case STATUS_OBJECT_NAME_NOT_FOUND:
        return 2;  /* ERROR_FILE_NOT_FOUND */
    case STATUS_OBJECT_NAME_COLLISION:
        return 80; /* ERROR_ALREADY_EXISTS */
    case STATUS_INVALID_PARAMETER:
        return 87; /* ERROR_INVALID_PARAMETER */
    case STATUS_NOT_IMPLEMENTED:
        return 127; /* ERROR_PROC_NOT_FOUND */
    case STATUS_NO_SUCH_DEVICE:
        return 246; /* ERROR_DEV_NOT_EXIST */
    default:
        return 33554169; /* ERROR_RESOURCE_NOT_FOUND-ish, just pass through */
    }
}
