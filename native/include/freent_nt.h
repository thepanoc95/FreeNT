#ifndef FREENT_NT_H
#define FREENT_NT_H

typedef unsigned char UCHAR;
typedef unsigned short USHORT;
typedef unsigned long ULONG;
typedef long NTSTATUS;
typedef unsigned long long ULONG_PTR;
typedef void *PVOID;
typedef void *HANDLE;
typedef unsigned short WCHAR;

#define NT_SUCCESS(Status) ((NTSTATUS)(Status) >= 0)
#define STATUS_SUCCESS ((NTSTATUS)0x00000000L)

typedef struct _IO_STATUS_BLOCK { NTSTATUS Status; ULONG_PTR Information; } IO_STATUS_BLOCK, *PIO_STATUS_BLOCK;
typedef struct _UNICODE_STRING { USHORT Length; USHORT MaximumLength; WCHAR *Buffer; } UNICODE_STRING;
typedef struct _OSVERSIONINFOW { ULONG dwOSVersionInfoSize; ULONG dwMajorVersion; ULONG dwMinorVersion; ULONG dwBuildNumber; ULONG dwPlatformId; WCHAR szCSDVersion[128]; } OSVERSIONINFOW;
typedef struct _RTL_USER_PROCESS_PARAMETERS {
    UCHAR Reserved0[0x20];
    HANDLE StandardInput;
    HANDLE StandardOutput;
    HANDLE StandardError;
    UCHAR Reserved1[0x38];
    UNICODE_STRING CommandLine;
} RTL_USER_PROCESS_PARAMETERS;
typedef struct _PEB_PARTIAL { UCHAR Reserved0[0x20]; RTL_USER_PROCESS_PARAMETERS *ProcessParameters; } PEB_PARTIAL;

__declspec(dllimport) NTSTATUS __stdcall NtTerminateProcess(HANDLE, NTSTATUS);
__declspec(dllimport) NTSTATUS __stdcall RtlGetVersion(OSVERSIONINFOW *);
__declspec(dllimport) NTSTATUS __stdcall NtReadFile(HANDLE, HANDLE, PIO_STATUS_BLOCK, PVOID, PIO_STATUS_BLOCK, PVOID, ULONG, ULONG, PVOID);
__declspec(dllimport) NTSTATUS __stdcall NtWriteFile(HANDLE, HANDLE, PIO_STATUS_BLOCK, PVOID, PIO_STATUS_BLOCK, PVOID, ULONG, ULONG, PVOID);

PEB_PARTIAL *freent_current_peb(void);
HANDLE freent_stdout(void);
HANDLE freent_stdin(void);
void freent_write(const char *text);
void freent_write_u32(ULONG value);
NTSTATUS freent_read_line(char *buffer, ULONG capacity, ULONG *length);
void freent_zero(void *buffer, ULONG length);
NTSTATUS freent_ndk_read_file(HANDLE handle, IO_STATUS_BLOCK *iosb, PVOID buffer, ULONG length);
NTSTATUS freent_ndk_write_file(HANDLE handle, IO_STATUS_BLOCK *iosb, PVOID buffer, ULONG length);
int freent_command_line_has(const WCHAR *word, USHORT word_length);
void freent_dispatch(void);
#endif
