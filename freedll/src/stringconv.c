/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         FreeNT FreeDLL
 * FILE:            freedll/src/stringconv.c
 * PURPOSE:         String conversion functions (ANSI <-> Unicode)
 * PROGRAMMER:      FreeNT Team
 */

#include "freedll.h"

/* Multi-byte to Unicode conversion (using system code page) */
NTSTATUS RtlMultiByteToUnicodeN(PWSTR Destination, ULONG DestinationCharCount,
                                PULONG DestinationBytes, PSTR Source, ULONG SourceLength)
{
    ULONG src_index = 0;
    ULONG dst_index = 0;
    ULONG bytes_written = 0;
    ULONG char_count = 0;

    if (Source == NULL || SourceLength == 0) {
        if (DestinationBytes)
            *DestinationBytes = 0;
        return STATUS_SUCCESS;
    }

    /* Simple conversion - assumes ASCII for now (each byte -> one WCHAR) */
    while (src_index < SourceLength && dst_index < DestinationCharCount) {
        unsigned char byte = (unsigned char)Source[src_index];

        /* Handle common escape sequences */
        if (byte == 0) {
            Destination[dst_index++] = 0;
            char_count++;
            bytes_written++;
            src_index++;
            break;
        }

        /* Simple byte-to-WCHAR conversion for ASCII range */
        Destination[dst_index++] = (WCHAR)byte;
        char_count++;
        bytes_written++;
        src_index++;
    }

    if (DestinationBytes)
        *DestinationBytes = bytes_written * sizeof(WCHAR);

    return STATUS_SUCCESS;
}

/* Unicode to multi-byte conversion */
NTSTATUS RtlUnicodeToMultiByteN(PSTR Destination, ULONG DestinationCharCount,
                                PULONG DestinationBytes, PWSTR Source, ULONG SourceLength)
{
    ULONG src_chars = 0;
    ULONG dst_bytes = 0;
    ULONG i;

    if (Source == NULL || SourceLength == 0) {
        if (DestinationBytes)
            *DestinationBytes = 0;
        return STATUS_SUCCESS;
    }

    /* Convert WCHARs to bytes (simple: just take low byte) */
    for (i = 0; i < SourceLength / sizeof(WCHAR) && dst_bytes < DestinationCharCount; i++) {
        if (Source[i] > 255) {
            /* Can't represent in single-byte - use '?' */
            Destination[dst_bytes++] = '?';
        } else {
            Destination[dst_bytes++] = (char)(Source[i] & 0xFF);
        }
    }

    if (DestinationBytes)
        *DestinationBytes = dst_bytes;

    return STATUS_SUCCESS;
}

/* GetModuleHandle implementation */
HMODULE GetModuleHandleA(LPCSTR lpModuleName)
{
    PPEB_SUBSET peb;
    PPEB_LDR_DATA ldr;
    PLDR_DATA_TABLE_ENTRY entry;
    LIST_ENTRY *list_entry;

    /* Get PEB from TEB */
    __asm__ volatile("movq %%gs:0x60, %0" : "=r"(peb));

    if (peb == NULL || peb->Ldr == NULL)
        return NULL;

    ldr = peb->Ldr;

    /* Search in load order */
    list_entry = ldr->InLoadOrderModuleList.Flink;
    while (list_entry != &ldr->InLoadOrderModuleList) {
        entry = (PLDR_DATA_TABLE_ENTRY)
            ((UCHAR *)list_entry - offsetof(LDR_DATA_TABLE_ENTRY, InLoadOrderLinks));

        if (lpModuleName == NULL)
            return (HMODULE)entry->DllBase;

        /* Compare base names */
        if (entry->BaseDllName.Buffer != NULL && entry->BaseDllName.Length > 0) {
            WCHAR search_name[256];
            ULONG i;
            for (i = 0; i < entry->BaseDllName.Length / sizeof(WCHAR) && lpModuleName[i]; i++) {
                search_name[i] = lpModuleName[i];
            }
            if (lpModuleName[i] == '\0' && i * sizeof(WCHAR) == entry->BaseDllName.Length) {
                search_name[i] = L'\0';
                if (freent_wcscmp(entry->BaseDllName.Buffer, search_name) == 0)
                    return (HMODULE)entry->DllBase;
            }
        }

        list_entry = list_entry->Flink;
    }

    return NULL;
}

HMODULE GetModuleHandleW(LPCWSTR lpModuleName)
{
    PPEB_SUBSET peb;
    PPEB_LDR_DATA ldr;
    PLDR_DATA_TABLE_ENTRY entry;
    LIST_ENTRY *list_entry;

    __asm__ volatile("movq %%gs:0x60, %0" : "=r"(peb));

    if (peb == NULL || peb->Ldr == NULL)
        return NULL;

    ldr = peb->Ldr;

    if (lpModuleName == NULL) {
        /* Get the first entry (the main module) */
        list_entry = ldr->InLoadOrderModuleList.Flink;
        if (list_entry != &ldr->InLoadOrderModuleList) {
            entry = (PLDR_DATA_TABLE_ENTRY)
                ((UCHAR *)list_entry - offsetof(LDR_DATA_TABLE_ENTRY, InLoadOrderLinks));
            return (HMODULE)entry->DllBase;
        }
        return NULL;
    }

    /* Search in load order */
    list_entry = ldr->InLoadOrderModuleList.Flink;
    while (list_entry != &ldr->InLoadOrderModuleList) {
        entry = (PLDR_DATA_TABLE_ENTRY)
            ((UCHAR *)list_entry - offsetof(LDR_DATA_TABLE_ENTRY, InLoadOrderLinks));

        if (entry->BaseDllName.Buffer != NULL && entry->BaseDllName.Length > 0) {
            if (freent_wcscmp(entry->BaseDllName.Buffer, lpModuleName) == 0)
                return (HMODULE)entry->DllBase;
        }

        list_entry = list_entry->Flink;
    }

    return NULL;
}

/* Sleep - delay execution */
VOID Sleep(DWORD dwMilliseconds)
{
    LARGE_INTEGER delay;
    /* Convert milliseconds to 100-nanosecond intervals (negative = relative) */
    delay.QuadPart = (LONGLONG)dwMilliseconds * -10000LL;
    NtDelayExecution(FALSE, &delay);
}

/* GetLastError - would use TEB->LastErrorValue in a real implementation */
DWORD GetLastError(VOID)
{
    DWORD last_error;
    __asm__ volatile("movl %%gs:0x68, %0" : "=r"(last_error));
    return last_error;
}

VOID SetLastError(DWORD dwErrCode)
{
    __asm__ volatile("movl %0, %%gs:0x68" :: "r"(dwErrCode));
}

/* LoadLibrary/LoadLibraryEx implementation */
HMODULE LoadLibraryA(LPCSTR lpLibFileName)
{
    UNICODE_STRING name;
    WCHAR name_buffer[MAX_PATH];
    ULONG len = 0;

    /* Convert ANSI to Unicode */
    while (lpLibFileName[len] && len < MAX_PATH - 1) {
        name_buffer[len] = (WCHAR)lpLibFileName[len];
        len++;
    }
    name_buffer[len] = L'\0';

    name.Length = (USHORT)(len * sizeof(WCHAR));
    name.MaximumLength = name.Length;
    name.Buffer = name_buffer;

    /* Would call LdrLoadDll in a real implementation */
    return NULL;
}

/* GetProcAddress - for now, return NULL (would need export tables) */
FARPROC GetProcAddress(HMODULE hModule, LPCSTR lpProcName)
{
    /* In a real implementation, this would search the module's export table */
    (void)hModule;
    (void)lpProcName;
    return NULL;
}
