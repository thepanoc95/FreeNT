/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         FreeNT FreeDLL
 * FILE:            freedll/src/rtl.c
 * PURPOSE:         RTL runtime library functions
 * PROGRAMMER:      FreeNT Team
 */

#include "freedll.h"

/* RTL version info */
#define RTL_LIB_VERSION "FreeDLL RTL - Tiny C Runtime for FreeNT"

/* ===== RTL Initialize / Shutdown ===== */

BOOLEAN RtlCreateUserThread(
    HANDLE hProcess,
    PSECURITY_DESCRIPTOR pSecurityDescriptor,
    BOOLEAN bCreateSuspended,
    ULONG dwCreationFlags,
    SIZE_T dwStackSize,
    SIZE_T dwMaximumStackSize,
    PVOID lpStartAddress,
    PVOID lpParameter,
    LPDWORD lpThreadId,
    LPTHREAD_START_ROUTINE lpThreadHandle
)
{
    /* In a real implementation, this would create a system thread.
       For now, return unsupported. */
    return FALSE;
}

/* ===== RTL Memory Operations ===== */

/* RtlMoveMemory - move memory (like memmove) */
VOID RtlMoveMemory(PVOID dest, PVOID src, SIZE_T length)
{
    freent_memmove(dest, src, length);
}

/* RtlCopyMemory - copy memory (like memcpy) */
VOID RtlCopyMemory(PVOID dest, PVOID src, SIZE_T length)
{
    freent_memcpy(dest, src, length);
}

/* RtlFillMemory - fill memory with a byte value */
VOID RtlFillMemory(PVOID dest, SIZE_T length, UCHAR fill)
{
    freent_memset(dest, fill, length);
}

/* RtlFillMemoryUShort - fill memory with a USHORT value */
VOID RtlFillMemoryUShort(PUSHORT dest, SIZE_T length, USHORT value)
{
    size_t i;
    for (i = 0; i < length / sizeof(USHORT); i++)
        dest[i] = value;
}

/* RtlZeroMemory - fill memory with zeros */
VOID RtlZeroMemory(PVOID dest, SIZE_T length)
{
    freent_memset(dest, 0, length);
}

/* RtlCompareMemory - compare memory blocks */
ULONG RtlCompareMemory(PVOID src1, PVOID src2, SIZE_T length)
{
    const UCHAR *p1 = (const UCHAR *)src1;
    const UCHAR *p2 = (const UCHAR *)src2;
    SIZE_T i;

    for (i = 0; i < length; i++) {
        if (p1[i] != p2[i])
            return i;
    }
    return length;
}

/* RtlCompareMemoryUpr - case insensitive memory compare */
ULONG RtlCompareMemoryUpr(PVOID src1, PVOID src2, SIZE_T length)
{
    const UCHAR *p1 = (const UCHAR *)src1;
    const UCHAR *p2 = (const UCHAR *)src2;
    SIZE_T i;

    for (i = 0; i < length; i++) {
        UCHAR c1 = (p1[i] >= 'a' && p1[i] <= 'z') ? p1[i] - 'a' + 'A' : p1[i];
        UCHAR c2 = (p2[i] >= 'a' && p2[i] <= 'z') ? p2[i] - 'a' + 'A' : p2[i];
        if (c1 != c2)
            return i;
    }
    return length;
}

/* ===== RTL String Operations ===== */

/* RtlMoveMemory equivalent for strings */
VOID RtlCopyUnicodeString(PUNICODE_STRING dest, PUNICODE_STRING src)
{
    if (dest == NULL || src == NULL)
        return;

    ULONG copy_length = src->Length;
    if (dest->MaximumLength < copy_length)
        copy_length = dest->MaximumLength;

    freent_memcpy(dest->Buffer, src->Buffer, copy_length);
    dest->Length = (USHORT)copy_length;
}

/* RtlDuplicateUnicodeString */
NTSTATUS RtlDuplicateUnicodeString(ULONG flags, PUNICODE_STRING src,
                                   PUNICODE_STRING dest)
{
    if (src == NULL || dest == NULL)
        return STATUS_INVALID_PARAMETER;

    ULONG length = src->Length;
    if (flags & 0x1) { /* RTL_DUPLICATE_UNICODE_STRING_NULLTERM */
        length += sizeof(WCHAR);
    }

    dest->MaximumLength = (USHORT)length;
    dest->Buffer = (PWSTR)RtlAllocateHeap(GetProcessHeap(), 0, length);
    if (dest->Buffer == NULL)
        return STATUS_NO_MEMORY;

    freent_memcpy(dest->Buffer, src->Buffer, src->Length);
    dest->Length = src->Length;
    if (length > src->Length)
        dest->Buffer[src->Length / sizeof(WCHAR)] = L'\0';

    return STATUS_SUCCESS;
}

/* RtlAppendUnicodeStringToString */
NTSTATUS RtlAppendUnicodeStringToString(PUNICODE_STRING dest, PUNICODE_STRING src)
{
    if (dest == NULL || src == NULL)
        return STATUS_INVALID_PARAMETER;

    if (dest->Length + src->Length > dest->MaximumLength)
        return STATUS_BUFFER_TOO_SMALL;

    freent_memcpy((UCHAR *)dest->Buffer + dest->Length, src->Buffer, src->Length);
    dest->Length += src->Length;
    return STATUS_SUCCESS;
}

/* RtlAppendStringToString */
NTSTATUS RtlAppendStringToString(PSTRING dest, PSTRING src)
{
    if (dest == NULL || src == NULL)
        return STATUS_INVALID_PARAMETER;

    if (dest->Length + src->Length > dest->MaximumLength)
        return STATUS_BUFFER_TOO_SMALL;

    freent_memcpy((UCHAR *)dest->Buffer + dest->Length, src->Buffer, src->Length);
    dest->Length += src->Length;
    return STATUS_SUCCESS;
}

/* RtlFormatCurrentUserKeyPath - returns registry path for user */
NTSTATUS RtlFormatCurrentUserKeyPath(PUNICODE_STRING Destination)
{
    static const WCHAR format[] = L"\\REGISTRY\\USER";
    if (Destination == NULL || Destination->MaximumLength < sizeof(format))
        return STATUS_BUFFER_TOO_SMALL;

    freent_wcscpy(Destination->Buffer, format);
    Destination->Length = sizeof(format) - sizeof(WCHAR);
    return STATUS_SUCCESS;
}

/* ===== RTL Time/Date ===== */

/* RtlSystemTimeToLocalTime */
NTSTATUS RtlSystemTimeToLocalTime(PLARGE_INTEGER SystemTime,
                                  PLARGE_INTEGER LocalTime)
{
    /* Would normally use system timezone bias */
    /* For simplicity, just copy */
    LocalTime->QuadPart = SystemTime->QuadPart;
    return STATUS_SUCCESS;
}

/* RtlTimeToTimeFields */
NTSTATUS RtlTimeToTimeFields(PLARGE_INTEGER Time, PTIME_FIELDS TimeFields)
{
    /* Convert 100-nanosecond intervals since 1601-01-01 to date/time fields */
    /* This is a simplified implementation */
    if (Time == NULL || TimeFields == NULL)
        return STATUS_INVALID_PARAMETER;

    /* In a complete implementation, this would convert the large integer
       to years, months, days, hours, minutes, seconds */
    freent_memset(TimeFields, 0, sizeof(TIME_FIELDS));
    return STATUS_SUCCESS;
}

/* RtlTimeFieldsToTime */
BOOLEAN RtlTimeFieldsToTime(PTIME_FIELDS TimeFields, PLARGE_INTEGER Time)
{
    if (Time == NULL)
        return FALSE;
    /* Simplified: returns failure for all inputs */
    return FALSE;
}

/* ===== RTL Version ===== */

LONG RtlGetVersion(PRTL_OSVERSIONINFOEXW VersionInformation)
{
    /* Return a minimal version (Windows NT compatible) */
    if (VersionInformation == NULL)
        return STATUS_INVALID_PARAMETER;

    if (VersionInformation->dwOSVersionInfoSize == sizeof(ULONG)) {
        /* Just a version number query */
        return 0;
    }

    /* For the full structure */
    VersionInformation->dwMajorVersion = 5;
    VersionInformation->dwMinorVersion = 2;
    VersionInformation->dwBuildNumber = 0;
    VersionInformation->dwPlatformId = 1; /* VER_PLATFORM_WIN32_NT */

    return 0; /* STATUS_SUCCESS */
}

/* ===== RTL Environment ===== */

/* GetEnvironmentVariable - simplified */
DWORD GetEnvironmentVariableA(LPCSTR lpName, LPSTR lpBuffer, DWORD nSize)
{
    /* For freestanding environment, environment variables come from PEB */
    /* Minimal implementation: return not found */
    (void)lpName;
    (void)lpBuffer;
    (void)nSize;
    return 0;
}

DWORD GetEnvironmentVariableW(LPCWSTR lpName, LPWSTR lpBuffer, DWORD nSize)
{
    (void)lpName;
    (void)lpBuffer;
    (void)nSize;
    return 0;
}

/* SetEnvironmentVariable */
BOOL SetEnvironmentVariableA(LPCSTR lpName, LPCSTR lpValue)
{
    (void)lpName;
    (void)lpValue;
    return FALSE;
}

BOOL SetEnvironmentVariableW(LPCWSTR lpName, LPCWSTR lpValue)
{
    (void)lpName;
    (void)lpValue;
    return FALSE;
}
