/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         FreeNT NTDYLIB
 * FILE:            ntdylib/src/dllmain.c
 * PURPOSE:         NTDYLIB DLL entry point and lifecycle
 * PROGRAMMER:      FreeNT Team
 */

#include "ntdylib.h"

/* Provide stub for MinGW stack probing (not needed in freestanding context) */
void ___chkstk_ms(void) {}

/* NTDYLIB internal state */
static BOOLEAN g_ntdylib_initialized = FALSE;

/*
 * NtdylibDllMain - Standard DLL entry point for ntdylib.dll.
 * Called by the system when the DLL is loaded or unloaded.
 * Delegates to NtdylibInit on process attach, and NtdylibCleanup on detach.
 */
BOOLEAN NTAPI NtdylibDllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    NTSTATUS status;

    switch (fdwReason) {
    case DLL_PROCESS_ATTACH:
        if (g_ntdylib_initialized)
            return TRUE;

        /* Initialize FreeDLL first (heap, strings, CRT) */
        GetProcessHeap();

        /* Initialize the module loader */
        status = NtdylibInit();
        if (!NT_SUCCESS(status))
            return FALSE;

        g_ntdylib_initialized = TRUE;
        break;

    case DLL_PROCESS_DETACH:
        if (!g_ntdylib_initialized)
            return TRUE;

        NtdylibCleanup();
        g_ntdylib_initialized = FALSE;
        break;

    case DLL_THREAD_ATTACH:
        /* No per-thread initialization needed */
        break;

    case DLL_THREAD_DETACH:
        /* No per-thread cleanup needed */
        break;
    }

    return TRUE;
}

/* Standard DllMain alias for Windows loader compatibility */
BOOLEAN NTAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    return NtdylibDllMain(hinstDLL, fdwReason, lpvReserved);
}

/* DllEntryPoint - alternative entry point name */
BOOLEAN NTAPI DllEntryPoint(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    return NtdylibDllMain(hinstDLL, fdwReason, lpvReserved);
}
