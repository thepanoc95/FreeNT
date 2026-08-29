/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         FreeNT FreeDLL
 * FILE:            freedll/include/freedll.h
 * PURPOSE:         FreeDLL main header - NT-compatible Tiny C Runtime
 * PROGRAMMER:      FreeNT Team
 */

#ifndef _FREEDLL_H
#define _FREEDLL_H

/*
 * This header provides a self-contained, freestanding implementation of
 * the ntdll.dll-compatible API plus a Tiny C Runtime.
 * It does NOT depend on the Windows SDK or CRT; it is fully self-contained.
 */

/* stddef.h equivalent for freestanding */
#ifndef _SIZE_T_DEFINED
#define _SIZE_T_DEFINED
#if defined(_M_X64) || defined(__x86_64__)
typedef unsigned long long   size_t;
#elif defined(_M_IX86) || defined(__i386__)
typedef unsigned long        size_t;
#endif
#endif

#ifndef NULL
#define NULL                 ((void*)0)
#endif

/* offsetof for freestanding */
#ifndef offsetof
#define offsetof(type, member) ((size_t)((unsigned char *)&(((type *)0)->member) - (unsigned char *)0))
#endif

/* ===== Basic Types ===== */
typedef unsigned char        UCHAR;
typedef signed short         CSHORT;
typedef unsigned short       USHORT;
typedef unsigned long        ULONG;
typedef signed long          LONG;
typedef void                 VOID;
typedef unsigned long long   ULONGLONG;
typedef long long            LONGLONG;
typedef long                 NTSTATUS;
typedef unsigned long long   ULONG_PTR;
typedef long long            LONG_PTR;
typedef unsigned long long   ULONGLONG_PTR;
typedef void                *PVOID;
typedef const void          *PCVOID;
typedef void                *HANDLE;
typedef unsigned short       WCHAR;
typedef unsigned char        BOOLEAN;
typedef int                  BOOL;
typedef unsigned char        BYTE;
typedef unsigned short       WORD;
typedef unsigned long        DWORD;
typedef WORD  *PWORD;
typedef BYTE  *PBYTE;
#if defined(_M_X64) || defined(__x86_64__)
typedef unsigned long long   SIZE_T;
typedef signed long long     SSIZE_T;
#else
typedef unsigned long        SIZE_T;
typedef signed long          SSIZE_T;
#endif
typedef ULONG_PTR             *PSIZE_T;
typedef ULONG_PTR            *PSIZE_T;
typedef ULONG                ACCESS_MASK;
typedef HANDLE               *PHANDLE;
typedef char                 CHAR;
typedef DWORD                *LPDWORD;
typedef DWORD                *PDWORD;
typedef ULONG                *PULONG;
typedef UCHAR                *PUCHAR;
typedef BOOLEAN              *PBOOLEAN;
typedef WCHAR                *PWSTR;
typedef const WCHAR          *PCWSTR;
typedef CHAR                 *PSTR;
typedef const CHAR           *PCSTR;
typedef CHAR                 *PCHAR;
typedef CHAR                 *LPSTR;
typedef const CHAR           *LPCSTR;
typedef WCHAR                *LPWSTR;
typedef const WCHAR          *LPCWSTR;
typedef PVOID                LPVOID;
typedef PVOID                *LPPVOID;
typedef PVOID                HMODULE;
typedef PVOID                HGLOBAL;
typedef PVOID                HLOCAL;

/* Constants */
#define TRUE                   1
#define FALSE                  0
typedef USHORT               *PUSHORT;
typedef const USHORT         *PCUSHORT;
typedef short                SHORT;
typedef SHORT                *PSHORT;
typedef float                FLOAT;
typedef FLOAT                *PFLOAT;
typedef LONG                 *PLONG;

/* NTAPI calling convention */
#if defined(_M_X64) || defined(__x86_64__)
#define NTAPI
#else
#define NTAPI __stdcall
#endif

/* FLS out-of-indexes sentinel */
#define FLS_OUT_OF_INDEXES    0xFFFFFFFF
#define MAX_PATH              260

/* Function pointer types */
typedef void                 (NTAPI *PTHREAD_START_ROUTINE)(PVOID);
typedef void                 (NTAPI *PNT_THREAD_START)(PVOID);
typedef PTHREAD_START_ROUTINE LPTHREAD_START_ROUTINE;
typedef PVOID                FARPROC;
typedef PVOID                *PPVOID;

/* va_list typedef - for freestanding, use compiler built-in */
typedef __builtin_va_list    va_list;
#define va_start(ap, param)    __builtin_va_start(ap, param)
#define va_arg(ap, type)       __builtin_va_arg(ap, type)
#define va_end(ap)             __builtin_va_end(ap)

/* Context structure - simplified for x64 */
typedef struct _CONTEXT {
    ULONG_PTR P1Home;
    ULONG_PTR P2Home;
    ULONG_PTR P3Home;
    ULONG_PTR P4Home;
    ULONG_PTR P5Home;
    ULONG_PTR P6Home;
    ULONG_PTR Spare0;
    ULONG_PTR Spare1;
    ULONG_PTR Spare2;
    ULONG_PTR Spare3;
    DWORD   ContextFlags;
    DWORD   Fill1[3];
    DWORD   Rax;
    DWORD   Rcx;
    DWORD   Rdx;
    DWORD   Rbx;
    DWORD   Rsp;
    DWORD   Rbp;
    DWORD   Rsi;
    DWORD   Rdi;
    DWORD   R8;
    DWORD   R9;
    DWORD   R10;
    DWORD   R11;
    DWORD   R12;
    DWORD   R13;
    DWORD   R14;
    DWORD   R15;
    DWORD   ReturnAddress;
} CONTEXT, *PCONTEXT;

/* Time fields structure */
typedef struct _TIME_FIELDS {
    CSHORT Year;
    CSHORT Month;
    CSHORT Day;
    CSHORT Hour;
    CSHORT Minute;
    CSHORT Second;
    CSHORT Milliseconds;
    CSHORT Weekday;
} TIME_FIELDS, *PTIME_FIELDS;

/* RTL OS version info */
typedef struct _RTL_OSVERSIONINFOEXW {
    ULONG dwOSVersionInfoSize;
    ULONG dwMajorVersion;
    ULONG dwMinorVersion;
    ULONG dwBuildNumber;
    ULONG dwPlatformId;
    WCHAR szCSDVersion[128];
} RTL_OSVERSIONINFOEXW, *PRTL_OSVERSIONINFOEXW;

/* NTSTATUS values */
#define STATUS_SUCCESS                   ((NTSTATUS)0x00000000L)
#define STATUS_UNSUCCESSFUL              ((NTSTATUS)0xC0000001L)
#define STATUS_INVALID_HANDLE            ((NTSTATUS)0xC0000008L)
#define STATUS_INVALID_PARAMETER         ((NTSTATUS)0xC000000DL)
#define STATUS_NO_SUCH_FILE              ((NTSTATUS)0xC000000FL)
#define STATUS_ACCESS_DENIED             ((NTSTATUS)0xC0000022L)
#define STATUS_NO_MEMORY                 ((NTSTATUS)0xC0000017L)
#define STATUS_OBJECT_NAME_NOT_FOUND     ((NTSTATUS)0xC0000034L)
#define STATUS_OBJECT_NAME_COLLISION     ((NTSTATUS)0xC0000035L)
#define STATUS_NOT_IMPLEMENTED           ((NTSTATUS)0xC0000002L)
#define STATUS_INVALID_PARAMETER_1       ((NTSTATUS)0xC000001DL)
#define STATUS_INVALID_PARAMETER_2       ((NTSTATUS)0xC000001EL)
#define STATUS_INVALID_PARAMETER_3       ((NTSTATUS)0xC000001FL)
#define STATUS_INVALID_PARAMETER_4       ((NTSTATUS)0xC0000020L)
#define STATUS_INVALID_PARAMETER_5       ((NTSTATUS)0xC0000021L)
#define STATUS_NO_SUCH_DEVICE            ((NTSTATUS)0xC000000EL)
#define STATUS_DEVICE_NOT_CONNECTED      ((NTSTATUS)0xC0000018L)
#define STATUS_DEVICE_BUSY               ((NTSTATUS)0xC00000C5L)
#define STATUS_BUFFER_TOO_SMALL          ((NTSTATUS)0xC0000023L)
#define STATUS_NOT_SUPPORTED             ((NTSTATUS)0xC00000BBL)
#define STATUS_PORT_MESSAGE_TOO_LONG     ((NTSTATUS)0xC0000044L)

#define NT_SUCCESS(Status)               ((NTSTATUS)(Status) >= 0)

/* NTSYSAPI for syscall declarations */
#define NTSYSAPI                           extern

/* ===== String / Memory Types ===== */
typedef struct _UNICODE_STRING {
    USHORT Length;
    USHORT MaximumLength;
    PWSTR  Buffer;
} UNICODE_STRING, *PUNICODE_STRING;

typedef struct _ANSI_STRING {
    USHORT Length;
    USHORT MaximumLength;
    PSTR   Buffer;
} ANSI_STRING, *PANSI_STRING;

typedef struct _OEM_STRING {
    USHORT Length;
    USHORT MaximumLength;
    PCHAR  Buffer;
} OEM_STRING, *POEM_STRING;

typedef OEM_STRING STRING;
typedef STRING *PSTRING;

#define RTL_CONSTANT_STRING(s) { sizeof(s) - 2, sizeof(s) - 2, (PWSTR)(s) }

/* ===== I/O Types ===== */
typedef struct _IO_STATUS_BLOCK {
    NTSTATUS Status;
    ULONG_PTR Information;
} IO_STATUS_BLOCK, *PIO_STATUS_BLOCK;

typedef struct _LARGE_INTEGER {
    union {
        struct {
            DWORD LowPart;
            LONG  HighPart;
        } DUMMYSTRUCTNAME;
        LONGLONG QuadPart;
    };
} LARGE_INTEGER, *PLARGE_INTEGER;

/* PEB/TEB definitions */
#define PEB32_SIZE         0x180
#define PEB64_SIZE         0x278
#define TEB32_SIZE         0x400
#define TEB64_SIZE         0x800

/* PEB (subset) - x64 layout */
typedef struct _PEB_SUBSET {
    BOOLEAN InheritedBorrowedToolsPresent;
    UCHAR   Pad0[0x07];
    PVOID   Mutant;
    PVOID   ImageBaseAddress;
    struct _PEB_LDR_DATA *Ldr;
    struct _RTL_USER_PROCESS_PARAMETERS *ProcessParameters;
} PEB_SUBSET, *PPEB_SUBSET;

/* TEB (subset) - x64 layout */
typedef struct _TEB_SUBSET {
    struct _EXCEPTION_REGISTRATION_RECORD *ExceptionList;
    PVOID   StackBase;
    PVOID   StackLimit;
    PVOID   SubStackTestError;
    PVOID   StaticSelf;
    PVOID   Self;
    /* ... other fields ... */
    struct _PEB_SUBSET *ProcessEnvironmentBlock;
} TEB_SUBSET, *PTEB_SUBSET;

/* Forward declarations for internal types */
typedef struct _LIST_ENTRY {
    struct _LIST_ENTRY *Flink;
    struct _LIST_ENTRY *Blink;
} LIST_ENTRY, *PLIST_ENTRY;

/* LDR Data */
typedef struct _LDR_DATA_TABLE_ENTRY {
    LIST_ENTRY  InLoadOrderLinks;
    LIST_ENTRY  InMemoryOrderLinks;
    LIST_ENTRY  InInitializationOrderLinks;
    PVOID       DllBase;
    PVOID       EntryPoint;
    ULONG       SizeOfFullImage;
    UNICODE_STRING FullDllName;
    UNICODE_STRING BaseDllName;
    ULONG       Flags;
    USHORT      LoadCount;
    USHORT      TlsIndex;
    union {
        LIST_ENTRY HashLinks;
        struct {
            ULONG       Timestamp;
            PVOID       CheckSum;
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME;
    /* ... more fields ... */
} LDR_DATA_TABLE_ENTRY, *PLDR_DATA_TABLE_ENTRY;

typedef struct _PEB_LDR_DATA {
    ULONG           Length;
    BOOLEAN         Initialized;
    PVOID           SsHandle;
    LIST_ENTRY      InLoadOrderModuleList;
    LIST_ENTRY      InMemoryOrderModuleList;
    LIST_ENTRY      InInitializationOrderModuleList;
    PVOID           EntryInProgress;
} PEB_LDR_DATA, *PPEB_LDR_DATA;

typedef struct _RTL_USER_PROCESS_PARAMETERS {
    ULONG           MaximumNumberOfThreads;
    ULONG           NumberOfThreads;
    UNICODE_STRING  CommandLine;
    HANDLE          hConsoleDevice;
    /* ... many more fields - we only need a subset ... */
    HANDLE          WindowTitle;
    HANDLE          DesktopInfo;
    HANDLE          ShellInfo;
    /* ... */
    WCHAR           CurrentWindowStation;
} RTL_USER_PROCESS_PARAMETERS, *PRTL_USER_PROCESS_PARAMETERS;

typedef struct _PEB {
    BOOLEAN                BeingDebugged;
    UCHAR                   SpareBool;
    UCHAR                   Pad[0x0A];
    /* ... more fields ... */
    union {
        HANDLE              Mutant;
        PVOID               Semaphore;
    } DUMMYUNIONNAME;
    PVOID                   ImageBaseAddress;
    /* ... */
    PPEB_LDR_DATA           Ldr;
    PRTL_USER_PROCESS_PARAMETERS ProcessParameters;
    /* ... more ... */
} PEB, *PPEB;

typedef struct _CLIENT_ID {
    HANDLE UniqueThread;
    HANDLE UniqueProcess;
} CLIENT_ID, *PCLIENT_ID;

/* Object Attributes */
#define OBJ_INHERIT             0x00000002L
#define OBJ_PERMANENT           0x00000008L
#define OBJ_EXCLUSIVE           0x00000010L
#define OBJ_CASE_INSENSITIVE    0x00000040L
#define OBJ_OPENIF              0x00000080L
#define OBJ_VALID_ATTRIBUTES    0x000001F8L

/* Forward declarations */
typedef struct _SID *PSID;
typedef struct _SECURITY_DESCRIPTOR *PSECURITY_DESCRIPTOR;
typedef struct _SECURITY_QUALITY_OF_SERVICE *PSECURITY_QUALITY_OF_SERVICE;

/* APC routine type */
typedef VOID (NTAPI *PIO_APC_ROUTINE)(PVOID ApcContext, PIO_STATUS_BLOCK IoStatusBlock, ULONG Reserved);

/* Object Attributes */

typedef struct _OBJECT_ATTRIBUTES {
    ULONG           Length;
    HANDLE          RootDirectory;
    PUNICODE_STRING ObjectName;
    ULONG           Attributes;
    PSID            SecurityDescriptor;
    PSECURITY_QUALITY_OF_SERVICE SecurityQualityOfService;
} OBJECT_ATTRIBUTES, *POBJECT_ATTRIBUTES;

/* Heap */
typedef struct _RTL_HEAP_PARAMETERS {
    ULONG       Length;
    PVOID       ReservePhase;
    PVOID       CommitPhase;
    PVOID       VirtualMemoryThreshold;
    PVOID       InitialNonVirtualSize;
    /* ... */
} RTL_HEAP_PARAMETERS, *PRTL_HEAP_PARAMETERS;

/* ===== FreeDLL internal definitions ===== */

/* HINSTANCE is just a handle for our purposes */
typedef HANDLE HINSTANCE;
typedef DWORD  LPVOID_DWORD;

/* DLL entry point type */
typedef BOOLEAN (NTAPI *PDLL_MAIN)(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved);

/* DLL entry reasons */
#define DLL_PROCESS_ATTACH    1
#define DLL_PROCESS_DETACH    0
#define DLL_THREAD_ATTACH     2
#define DLL_THREAD_DETACH     3

/* TLS Callbacks */
typedef VOID (NTAPI *PTLS_CALLBACK_FUNCTION)(PVOID DllBase, DWORD Reason, PVOID Context);

/* ===== Tiny C Runtime declarations ===== */

/* Memory management */
extern void *freent_memcpy(void *dest, const void *src, size_t n);
extern void *freent_memmove(void *dest, const void *src, size_t n);
extern void *freent_memset(void *s, int c, size_t n);
extern int   freent_memcmp(const void *s1, const void *s2, size_t n);
extern void *freent_memchr(const void *s, int c, size_t n);

/* String functions */
extern size_t freent_strlen(const char *s);
extern size_t freent_strnlen(const char *s, size_t maxlen);
extern char *freent_strcpy(char *dest, const char *src);
extern char *freent_strncpy(char *dest, const char *src, size_t n);
extern char *freent_strcat(char *dest, const char *src);
extern char *freent_strncat(char *dest, const char *src, size_t n);
extern int   freent_strcmp(const char *s1, const char *s2);
extern int   freent_strncmp(const char *s1, const char *s2, size_t n);
extern char *freent_strchr(const char *s, int c);
extern char *freent_strrchr(const char *s, int c);
extern char *freent_strstr(const char *haystack, const char *needle);
extern char *freent_strtok(char *s, const char *delim);
extern char *freent_strdup(const char *s);
extern size_t freent_strspn(const char *s, const char *accept);
extern size_t freent_strcspn(const char *s, const char *reject);
extern char *freent_strpbrk(const char *s, const char *accept);

/* Wide string functions */
extern size_t freent_wcslen(const WCHAR *s);
extern int   freent_wcscmp(const WCHAR *s1, const WCHAR *s2);
extern int   freent_wcsncmp(const WCHAR *s1, const WCHAR *s2, size_t n);
extern WCHAR *freent_wcscpy(WCHAR *dest, const WCHAR *src);
extern WCHAR *freent_wcsncpy(WCHAR *dest, const WCHAR *src, size_t n);

/* Standard library */
extern int   freent_atoi(const char *nptr);
extern long  freent_atol(const char *nptr);
extern long long freent_atoll(const char *nptr);
extern int   freent_abs(int j);
extern long  freent_labs(long j);
extern void *freent_malloc(size_t size);
extern void freent_free(void *ptr);
extern void *freent_calloc(size_t count, size_t size);
extern void *freent_realloc(void *ptr, size_t size);
extern void *freent_bsearch(const void *key, const void *base, size_t count, size_t size,
                            int (*compar)(const void *, const void *));
extern int freent_rand(void);
extern void freent_srand(unsigned int seed);

/* Sorting */
extern void freent_qsort(void *base, size_t nmemb, size_t size,
                         int (*compar)(const void *, const void *));

/* ===== NT Syscall Imports (from ntdll.dll) ===== */
/*
 * These are the syscalls that FreeDLL relies on from ntdll.dll.
 * FreeDLL itself implements the CRT, heap, and RTL functions, but
 * delegates actual kernel calls to ntdll.
 */
__declspec(dllimport)
NTSTATUS NTAPI
NtTerminateProcess(HANDLE ProcessHandle, NTSTATUS ExitStatus);

__declspec(dllimport)
NTSTATUS NTAPI
NtDelayExecution(BOOLEAN Alertable, PLARGE_INTEGER DelayInterval);

__declspec(dllimport)
NTSTATUS NTAPI
NtAllocateVirtualMemory(
    HANDLE ProcessHandle,
    PVOID *BaseAddress,
    PVOID *ZeroBits,
    PSIZE_T RegionSize,
    ULONG AllocationType,
    ULONG Protect
);

__declspec(dllimport)
NTSTATUS NTAPI
NtFreeVirtualMemory(
    HANDLE ProcessHandle,
    PVOID *BaseAddress,
    PSIZE_T RegionSize,
    ULONG FreeType
);

/* RtlGetVersion is exported by FreeDLL itself, not imported from ntdll */

/* ===== Memory allocation constants ===== */
#define MEM_COMMIT      0x00001000
#define MEM_RESERVE     0x00002000
#define MEM_RELEASE     0x00008000
#define MEM_FREE        0x00010000
#define PAGE_READONLY   0x02
#define PAGE_READWRITE  0x04
#define PAGE_WRITECOPY  0x08
#define PAGE_EXECUTE    0x10
#define PAGE_EXECUTE_READ  0x20
#define PAGE_EXECUTE_READWRITE  0x40
#define PAGE_EXECUTE_WRITECOPY  0x80

/* Object attributes */
#define OBJ_INHERIT             0x00000002L
#define OBJ_PERMANENT           0x00000008L
#define OBJ_EXCLUSIVE           0x00000010L
#define OBJ_CASE_INSENSITIVE    0x00000040L
#define OBJ_OPENIF              0x00000080L
#define OBJ_KERNEL_HANDLE       0x0000200L

/* NT I/O status block (already defined above as IO_STATUS_BLOCK) */
#define FILE_SYNCHRONOUS_IO_NONALERT 0x00000020
#define FILE_NON_BLOCKING            0x00000080

__declspec(dllimport)
NTSTATUS NTAPI
NtCreateFile(
    PHANDLE FileHandle,
    ACCESS_MASK DesiredAccess,
    POBJECT_ATTRIBUTES ObjectAttributes,
    PIO_STATUS_BLOCK IoStatusBlock,
    PLARGE_INTEGER AllocationSize,
    ULONG FileAttributes,
    ULONG ShareAccess,
    ULONG CreateDisposition,
    ULONG CreateOptions,
    PVOID EaBuffer,
    ULONG EaLength
);

/* ===== FreeDLL public API (NT-compatible ntdll exports) ===== */

/* Heap */
extern HANDLE RtlCreateHeap(ULONG Flags, PVOID Address, SIZE_T ReserveSize,
                            SIZE_T CommitSize, PVOID PSLock, PRTL_HEAP_PARAMETERS Parameters);
extern BOOLEAN RtlDestroyHeap(HANDLE HeapHandle);
extern PVOID RtlAllocateHeap(HANDLE HeapHandle, ULONG Flags, SIZE_T Size);
extern PVOID RtlReAllocateHeap(HANDLE HeapHandle, ULONG Flags, PVOID HeapBase, SIZE_T Size);
extern BOOLEAN RtlFreeHeap(HANDLE HeapHandle, ULONG Flags, PVOID HeapBase);
extern SIZE_T RtlSizeHeap(HANDLE HeapHandle, ULONG Flags, PVOID HeapBase);
BOOLEAN InitializeProcessHeap(VOID);

/* Process/Thread */
extern HANDLE  GetCurrentProcess(VOID);
extern HANDLE  GetCurrentThread(VOID);
extern DWORD   GetCurrentProcessId(VOID);
extern DWORD   GetCurrentThreadId(VOID);

/* Exit */
extern VOID    ExitProcess(DWORD ExitCode);
extern VOID    RtlExitUserProcess(NTSTATUS Status);

/* String conversion */
extern NTSTATUS RtlMultiByteToUnicodeN(PWSTR Destination, ULONG DestinationCharCount,
                                       PULONG DestinationBytes, PSTR Source, ULONG SourceLength);
extern NTSTATUS RtlUnicodeToMultiByteN(PSTR Destination, ULONG DestinationCharCount,
                                       PULONG DestinationBytes, PWSTR Source, ULONG SourceLength);

/* Get module handle - implemented locally by FreeDLL */
HMODULE GetModuleHandleA(LPCSTR lpModuleName);
HMODULE GetModuleHandleW(LPCWSTR lpModuleName);

/* Sleep - implemented locally by FreeDLL */
VOID  Sleep(DWORD dwMilliseconds);

/* ===== RTL OS Version ===== */
/* RtlGetVersion is implemented locally in FreeDLL (rtl.c) */

/* ===== Environment ===== */
DWORD GetEnvironmentVariableA(LPCSTR lpName, LPSTR lpBuffer, DWORD nSize);
DWORD GetEnvironmentVariableW(LPCWSTR lpName, LPWSTR lpBuffer, DWORD nSize);
BOOL SetEnvironmentVariableA(LPCSTR lpName, LPCSTR lpValue);
BOOL SetEnvironmentVariableW(LPCWSTR lpName, LPCWSTR lpValue);

/* ===== RTL Memory ===== */
extern VOID RtlMoveMemory(PVOID dest, PVOID src, SIZE_T length);
extern VOID RtlCopyMemory(PVOID dest, PVOID src, SIZE_T length);
extern VOID RtlFillMemory(PVOID dest, SIZE_T length, UCHAR fill);
extern VOID RtlFillMemoryUShort(PUSHORT dest, SIZE_T length, USHORT value);
extern VOID RtlZeroMemory(PVOID dest, SIZE_T length);
extern ULONG RtlCompareMemory(PVOID src1, PVOID src2, SIZE_T length);
extern ULONG RtlCompareMemoryUpr(PVOID src1, PVOID src2, SIZE_T length);

/* ===== RTL String ===== */
extern VOID RtlCopyUnicodeString(PUNICODE_STRING dest, PUNICODE_STRING src);
extern NTSTATUS RtlDuplicateUnicodeString(ULONG flags, PUNICODE_STRING src, PUNICODE_STRING dest);
extern NTSTATUS RtlAppendUnicodeStringToString(PUNICODE_STRING dest, PUNICODE_STRING src);
extern NTSTATUS RtlAppendStringToString(PSTRING dest, PSTRING src);
extern NTSTATUS RtlFormatCurrentUserKeyPath(PUNICODE_STRING Destination);

/* ===== RTL Time ===== */
extern NTSTATUS RtlSystemTimeToLocalTime(PLARGE_INTEGER SystemTime, PLARGE_INTEGER LocalTime);
extern NTSTATUS RtlTimeToTimeFields(PLARGE_INTEGER Time, PTIME_FIELDS TimeFields);
extern BOOLEAN RtlTimeFieldsToTime(PTIME_FIELDS TimeFields, PLARGE_INTEGER Time);

/* ===== Heap ===== */
extern HANDLE GetProcessHeap(VOID);
extern HANDLE CreateHeap(ULONG flags, SIZE_T reserve_size, SIZE_T commit_size);
extern BOOLEAN DestroyHeap(HANDLE heap);

/* ===== Process utilities ===== */
DWORD  GetTickCount(VOID);
BOOLEAN QueryPerformanceCounter(PLARGE_INTEGER lpPerformanceCount);
BOOLEAN QueryPerformanceFrequency(PLARGE_INTEGER lpFrequency);
BOOLEAN GetProcessShutdownParameters(LPDWORD lpdwFlags, HANDLE *lphand);

/* ===== Error handling ===== */
DWORD GetLastError(VOID);
VOID  SetLastError(DWORD dwErrCode);
extern ULONG RtlNtStatusToDosError(NTSTATUS Status);

/* ===== Exception ===== */
extern VOID RtlCaptureContext(PCONTEXT ContextRecord);

/* ===== Module management ===== */
HMODULE LoadLibraryA(LPCSTR lpLibFileName);
FARPROC GetProcAddress(HMODULE hModule, LPCSTR lpProcName);

/* ===== TLS/FLS ===== */
extern BOOLEAN RegisterTlsCallback(PTLS_CALLBACK_FUNCTION callback);
extern DWORD FlsAlloc(PVOID pCallback);
extern PVOID FlsGetValue(DWORD dwFlsIndex);
extern BOOLEAN FlsSetValue(DWORD dwFlsIndex, PVOID lpFlsData);
extern BOOLEAN FlsFree(DWORD dwFlsIndex);

/* ===== CRT Standard Library ===== */
extern void *memcpy(void *dest, const void *src, size_t n);
extern void *memmove(void *dest, const void *src, size_t n);
extern void *memset(void *s, int c, size_t n);
extern int   memcmp(const void *s1, const void *s2, size_t n);
extern void *memchr(const void *s, int c, size_t n);

extern size_t strlen(const char *s);
extern size_t strnlen(const char *s, size_t maxlen);
extern char *strcpy(char *dest, const char *src);
extern char *strncpy(char *dest, const char *src, size_t n);
extern char *strcat(char *dest, const char *src);
extern char *strncat(char *dest, const char *src, size_t n);
extern int   strcmp(const char *s1, const char *s2);
extern int   strncmp(const char *s1, const char *s2, size_t n);
extern char *strchr(const char *s, int c);
extern char *strrchr(const char *s, int c);
extern char *strstr(const char *haystack, const char *needle);
extern char *strtok(char *s, const char *delim);
extern char *strdup(const char *s);
extern size_t strspn(const char *s, const char *accept);
extern size_t strcspn(const char *s, const char *reject);
extern char *strpbrk(const char *s, const char *accept);

extern size_t wcslen(const WCHAR *s);
extern int   wcscmp(const WCHAR *s1, const WCHAR *s2);
extern int   wcsncmp(const WCHAR *s1, const WCHAR *s2, size_t n);
extern WCHAR *wcscpy(WCHAR *dest, const WCHAR *src);
extern WCHAR *wcsncpy(WCHAR *dest, const WCHAR *src, size_t n);

extern int   atoi(const char *nptr);
extern long  atol(const char *nptr);
extern long long atoll(const char *nptr);
extern int   abs(int j);
extern long  labs(long j);
extern void *malloc(size_t size);
extern void free(void *ptr);
extern void *calloc(size_t count, size_t size);
extern void *realloc(void *ptr, size_t size);
extern void *bsearch(const void *key, const void *base, size_t count, size_t size,
                     int (*compar)(const void *, const void *));
extern int rand(void);
extern void srand(unsigned int seed);
extern void  qsort(void *base, size_t nmemb, size_t size, int (*compar)(const void *, const void *));

extern int snprintf(char *buffer, size_t size, const char *format, ...);
extern int vsnprintf(char *buffer, size_t size, const char *format, va_list ap);

/* Freestanding CRT - freent_ prefixed implementations */
extern int freent_vsnprintf(char *buffer, size_t size, const char *format, va_list ap);
extern int freent_snprintf(char *buffer, size_t size, const char *format, ...);

extern int atexit(void (*func)(void));
extern int __cxa_atexit(void (*func)(void *), void *arg, void *dso_handle);

/* ===== Data Exports ===== */
extern int   __argc;
extern char **__argv;
extern WCHAR **__wargv;
extern char **__environ;
extern ULONG_PTR fls_index;

/* Export macro for applications using FreeDLL */
#define FREEDLL_EXPORT __declspec(dllexport)
#define FREEDLL_IMPORT __declspec(dllimport)

#endif /* _FREEDLL_H */
