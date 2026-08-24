/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         FreeNT NTDYLIB
 * FILE:            ntdylib/include/ntdylib.h
 * PURPOSE:         NT Dynamic Library Loader - uses FreeDLL services
 * PROGRAMMER:      FreeNT Team
 */

#ifndef _NTDYLIB_H
#define _NTDYLIB_H

/*
 * NTDYLIB provides dynamic library loading for NT-based systems.
 * It uses FreeDLL's services for memory management, string operations,
 * and system calls. NTDYLIB parses PE (Portable Executable) format
 * to locate and load DLLs, resolve imports, and expose exports.
 */

/* Include FreeDLL for memory and string services */
#include "freedll.h"

/* ===== PE Format Definitions ===== */

/* Standard PE header values */
#define IMAGE_DOS_SIGNATURE             0x5A4D      /* "MZ" */
#define IMAGE_NT_SIGNATURE              0x00004554L  /* "PE\0\0" */
#define IMAGE_NT_OPTIONAL_HDR32_MAGIC   0x10b
#define IMAGE_NT_OPTIONAL_HDR64_MAGIC   0x20b
#define IMAGE_FILE_DLL                  0x2000
#define IMAGE_FILE_EXECUTABLE_IMAGE     0x0002

/* PE Characteristics */
#define IMAGE_FILE_RELOCS_STRIPPED      0x0001
#define IMAGE_FILE_DEBUG_STRIPPED       0x0200
#define IMAGE_FILE_DLL                  0x2000

/* DLL Characteristics */
#define IMAGE_DLLCHARACTERISTICS_DYNAMIC_BASE 0x0040
#define IMAGE_DLLCHARACTERISTICS_NX_COMPAT    0x0100

/* Directory constants */
#define IMAGE_NUMBEROF_DIRECTORY_ENTRIES    16
#define IMAGE_DIRECTORY_ENTRY_EXPORT        0
#define IMAGE_DIRECTORY_ENTRY_IMPORT        1
#define IMAGE_DIRECTORY_ENTRY_BASERELOC     2

/* Data directory */
typedef struct _IMAGE_DATA_DIRECTORY {
    DWORD VirtualAddress;
    DWORD Size;
} IMAGE_DATA_DIRECTORY, *PIMAGE_DATA_DIRECTORY;

/* Export directory */
typedef struct _IMAGE_EXPORT_DIRECTORY {
    DWORD Characteristics;
    DWORD TimeDateStamp;
    WORD  MajorVersion;
    WORD  MinorVersion;
    DWORD Name;
    DWORD Base;
    DWORD NumberOfFunctions;
    DWORD NumberOfNames;
    DWORD AddressOfFunctions;
    DWORD AddressOfNames;
    DWORD AddressOfNameOrdinals;
} IMAGE_EXPORT_DIRECTORY, *PIMAGE_EXPORT_DIRECTORY;

/* Import address table entry */
typedef struct _IMAGE_THUNK_DATA64 {
    union {
        ULONGLONG ForwarderString;
        ULONGLONG Function;
        ULONGLONG Ordinal;
        ULONGLONG AddressOfData;
    } u1;
} IMAGE_THUNK_DATA64;

typedef struct _IMAGE_THUNK_DATA32 {
    union {
        DWORD ForwarderString;
        DWORD Function;
        DWORD Ordinal;
        DWORD AddressOfData;
    } u1;
} IMAGE_THUNK_DATA32;

typedef IMAGE_THUNK_DATA64 IMAGE_THUNK_DATA;
typedef IMAGE_THUNK_DATA   *PIMAGE_THUNK_DATA;
typedef IMAGE_THUNK_DATA   *PIMG_TNB;
typedef IMAGE_THUNK_DATA64 *PIMAGE_THUNK_DATA64;
typedef IMAGE_THUNK_DATA32 *PIMAGE_THUNK_DATA32;

/* Import descriptor */
typedef struct _IMAGE_IMPORT_DESCRIPTOR {
    union {
        DWORD Characteristics;
        DWORD OriginalFirstThunk;
    } u;
    DWORD TimeDateStamp;
    DWORD ForwarderChain;
    DWORD Name;
    DWORD FirstThunk;
} IMAGE_IMPORT_DESCRIPTOR, *PIMAGE_IMPORT_DESCRIPTOR;

#define IMAGE_NO_THUNK_INDEX ((DWORD)-1)
#define IMAGE_SNAP_BY_ORDINAL64(ordinal) ((ordinal & 0x8000000000000000ULL) != 0)
#define IMAGE_ORDINAL16(ordinal) ((WORD)(ordinal & 0xffff))
#define IMAGE_ORDINAL32(ordinal) ((DWORD)(ordinal & 0xffff))

/* Additional status codes */
#define STATUS_END_OF_FILE    ((NTSTATUS)0xC0000014L)

/* Section header */
typedef struct _IMAGE_SECTION_HEADER {
    UCHAR   Name[8];
    union {
        DWORD PhysicalAddress;
        DWORD VirtualSize;
    } Misc;
    DWORD   VirtualAddress;
    DWORD   SizeOfRawData;
    DWORD   PointerToRawData;
    DWORD   PointerToRelocations;
    DWORD   PointerToLinenumbers;
    WORD    NumberOfRelocations;
    WORD    NumberOfLinenumbers;
    DWORD   Characteristics;
} IMAGE_SECTION_HEADER, *PIMAGE_SECTION_HEADER;

#define IMAGE_SCN_MEM_EXECUTE     0x20000000
#define IMAGE_SCN_MEM_READ          0x40000000
#define IMAGE_SCN_MEM_WRITE         0x80000000

/* Relocation types */
#define IMAGE_REL_BASED_HIGHLOW         0x00000001
#define IMAGE_REL_BASED_DIR64           0x00000000
#define IMAGE_REL_BASED_ABSOLUTE        0x00000000

/* Base relocation */
typedef struct _IMAGE_BASE_RELOCATION {
    DWORD VirtualAddress;
    DWORD SizeOfBlock;
    /* DWORD TypeOffset[] follows */
} IMAGE_BASE_RELOCATION, *PIMAGE_BASE_RELOCATION;

#define RELOC_TARGET_SIZE_32  0x3
#define RELOC_TARGET_SIZE_64  0x4

/* File header */
typedef struct _IMAGE_FILE_HEADER {
    WORD    Machine;
    WORD    NumberOfSections;
    DWORD   TimeDateStamp;
    DWORD   PointerToSymbolTable;
    DWORD   NumberOfSymbols;
    WORD    SizeOfOptionalHeader;
    WORD    Characteristics;
} IMAGE_FILE_HEADER, *PIMAGE_FILE_HEADER;

/* Optional header (x64) */
typedef struct _IMAGE_OPTIONAL_HEADER64 {
    WORD        Magic;
    BYTE        MajorLinkerVersion;
    BYTE        MinorLinkerVersion;
    DWORD       SizeOfCode;
    DWORD       SizeOfInitializedData;
    DWORD       SizeOfUninitializedData;
    DWORD       AddressOfEntryPoint;
    DWORD       BaseOfCode;
    ULONGLONG   ImageBase;
    DWORD       SectionAlignment;
    DWORD       FileAlignment;
    DWORD       MajorOperatingSystemVersion;
    DWORD       MinorOperatingSystemVersion;
    DWORD       MajorImageVersion;
    DWORD       MinorImageVersion;
    DWORD       MajorSubsystemVersion;
    DWORD       MinorSubsystemVersion;
    DWORD       Win32VersionValue;
    DWORD       SizeOfImage;
    DWORD       SizeOfHeaders;
    DWORD       CheckSum;
    WORD        NumberOfRvaAndSizes;
    IMAGE_DATA_DIRECTORY DataDirectory[IMAGE_NUMBEROF_DIRECTORY_ENTRIES];
} IMAGE_OPTIONAL_HEADER64, *PIMAGE_OPTIONAL_HEADER64;

/* Optional header (x86) */
typedef struct _IMAGE_OPTIONAL_HEADER32 {
    WORD   Magic;
    BYTE   MajorLinkerVersion;
    BYTE   MinorLinkerVersion;
    DWORD  SizeOfCode;
    DWORD  SizeOfInitializedData;
    DWORD  SizeOfUninitializedData;
    DWORD  AddressOfEntryPoint;
    DWORD  BaseOfCode;
    DWORD  BaseOfData;
    DWORD  ImageBase;
    DWORD  SectionAlignment;
    DWORD  FileAlignment;
    DWORD  MajorOperatingSystemVersion;
    DWORD  MinorOperatingSystemVersion;
    DWORD  MajorImageVersion;
    DWORD  MinorImageVersion;
    DWORD  MajorSubsystemVersion;
    DWORD  MinorSubsystemVersion;
    DWORD  Win32VersionValue;
    DWORD  SizeOfImage;
    DWORD  SizeOfHeaders;
    DWORD  CheckSum;
    DWORD  NumberOfRvaAndSizes;
    IMAGE_DATA_DIRECTORY DataDirectory[IMAGE_NUMBEROF_DIRECTORY_ENTRIES];
} IMAGE_OPTIONAL_HEADER32, *PIMAGE_OPTIONAL_HEADER32;

/* DOS header */
typedef struct _IMAGE_DOS_HEADER {
    WORD   e_magic;
    WORD   e_cblp;
    WORD   e_cp;
    WORD   e_crlc;
    WORD   e_cputype;
    WORD   e_minalloc;
    WORD   e_maxalloc;
    WORD   e_ss;
    WORD   e_sp;
    WORD   e_csum;
    WORD   e_ip;
    WORD   e_cs;
    WORD   e_lfarlc;
    WORD   e_ovno;
    WORD   e_oeminfo[4];
    WORD   e_lfanew;
} IMAGE_DOS_HEADER, *PIMAGE_DOS_HEADER;

/* NT Headers (x64) */
typedef struct _IMAGE_NT_HEADERS64 {
    DWORD                   Signature;
    IMAGE_FILE_HEADER       FileHeader;
    IMAGE_OPTIONAL_HEADER64 OptionalHeader;
} IMAGE_NT_HEADERS64, *PIMAGE_NT_HEADERS64;

/* NT Headers (x86) */
typedef struct _IMAGE_NT_HEADERS32 {
    DWORD                  Signature;
    IMAGE_FILE_HEADER      FileHeader;
    IMAGE_OPTIONAL_HEADER32 OptionalHeader;
} IMAGE_NT_HEADERS32, *PIMAGE_NT_HEADERS32;

/* ===== NTDYLIB API ===== */

/* DLL load flags */
#define NTDLL_LOAD_LIBRARY_AS_IMAGE_DATA      0x00000001
#define NTDLL_LOAD_LIBRARY_AS_DATAFILE        0x00000002
#define NTDLL_LOAD_LIBRARY_SEARCH_DEFAULT_DIRS 0x00000100

/* Loaded module entry */
typedef struct _NTDYLIB_MODULE_ENTRY {
    UNICODE_STRING FullPathName;
    UNICODE_STRING BaseName;
    PVOID         BaseAddress;
    DWORD         SizeOfImage;
    DWORD         Flags;
    ULONG_PTR     ReferenceCount;
    LIST_ENTRY    InLoadOrderLinks;
    struct _NTDYLIB_MODULE_ENTRY *Next;
} NTDYLIB_MODULE_ENTRY, *PNTDYLIB_MODULE_ENTRY;

/* Forward declaration */
struct _NTDYLIB_MODULE;
typedef struct _NTDYLIB_MODULE {
    PVOID                  BaseAddress;
    UNICODE_STRING         FullPath;
    UNICODE_STRING         BaseName;
    DWORD                  SizeOfImage;
    DWORD                  Flags;
    ULONG_PTR              ReferenceCount;
    PLIST_ENTRY            InLoadOrderLinks;
    struct _NTDYLIB_MODULE  *Next;
    PNTDYLIB_MODULE_ENTRY  Entry;
    PIMAGE_NT_HEADERS64    Headers;
} NTDYLIB_MODULE, *PNTDYLIB_MODULE;

/* Internal state */
extern PNTDYLIB_MODULE g_LoadedModules;
extern BOOLEAN          g_NtdylibInitialized;

/* Exported function pointer type */
typedef PVOID (NTAPI *PNTDYLIB_EXPORT_LOOKUP)(PNTDYLIB_MODULE Module, PCSTR Name);

/*
 * NtdylibLoadDll - Load a DLL from a file path into the current process.
 * Returns a handle to the loaded module, or NULL on failure.
 * Uses FreeDLL for memory allocation, string operations, and system calls.
 */
extern PNTDYLIB_MODULE NtdylibLoadDll(LPCWSTR lpLibFileName, DWORD dwFlags);

/*
 * NtdylibGetProcAddress - Resolve an exported function by name or ordinal.
 * Returns a pointer to the function, or NULL on failure.
 */
extern FARPROC NtdylibGetProcAddress(PNTDYLIB_MODULE Module, LPCSTR lpProcName);

/*
 * NtdylibUnloadDll - Unload a previously loaded DLL.
 * Returns TRUE on success, FALSE on failure.
 */
extern BOOLEAN NtdylibUnloadDll(PNTDYLIB_MODULE Module);

/*
 * NtdylibGetModuleHandle - Find a loaded module by name.
 * Returns the module handle if loaded, NULL otherwise.
 */
extern PNTDYLIB_MODULE NtdylibGetModuleHandle(LPCWSTR lpModuleName);

/*
 * NtdylibInit - Initialize the module loader (called during process startup).
 * Returns STATUS_SUCCESS on success.
 */
extern NTSTATUS NtdylibInit(VOID);

/*
 * NtdylibCleanup - Clean up all loaded modules (called during process exit).
 */
extern VOID NtdylibCleanup(VOID);

/*
 * NtdylibGetModules - Enumerate loaded modules.
 * Returns the number of modules and fills the array.
 */
extern ULONG NtdylibGetModules(PNTDYLIB_MODULE *Modules, ULONG MaxCount);

/*
 * NtdylibGetModuleInfo - Get information about a loaded module.
 */
extern NTSTATUS NtdylibGetModuleInfo(PNTDYLIB_MODULE Module, PVOID Info, ULONG InfoSize);

/* ===== Internal PE loader functions ===== */

/*
 * NtdylibParseHeaders - Parse PE headers of a loaded image.
 * Returns STATUS_SUCCESS on success.
 */
extern NTSTATUS NtdylibParseHeaders(PVOID ImageBase, PIMAGE_NT_HEADERS64 *Headers);

/*
 * NtdylibResolveImports - Resolve all imports for a loaded module.
 */
extern NTSTATUS NtdylibResolveImports(PNTDYLIB_MODULE Module);

/*
 * NtdylibProcessRelocs - Apply relocations to a loaded module.
 */
extern NTSTATUS NtdylibProcessRelocs(PNTDYLIB_MODULE Module, PVOID LoadBase);

/*
 * NtdylibFindExport - Find an export by name or ordinal.
 */
extern PVOID NtdylibFindExport(PNTDYLIB_MODULE Module, PCSTR Name, WORD Ordinal);

/* ===== FreeDLL Integration ===== */

/* FreeDLL provides the underlying primitives used by ntdylib */
extern HANDLE GetProcessHeap(VOID);
extern PVOID  RtlAllocateHeap(HANDLE HeapHandle, ULONG Flags, SIZE_T Size);
extern BOOLEAN RtlFreeHeap(HANDLE HeapHandle, ULONG Flags, PVOID HeapBase);
extern VOID   RtlCopyMemory(PVOID dest, PVOID src, SIZE_T length);
extern VOID   RtlZeroMemory(PVOID dest, SIZE_T length);
extern VOID   RtlMoveMemory(PVOID dest, PVOID src, SIZE_T length);
extern size_t freent_strlen(const char *s);
extern char  *freent_strcpy(char *dest, const char *src);
extern int   freent_strcmp(const char *s1, const char *s2);
extern size_t freent_wcslen(const WCHAR *s);
extern int   freent_wcscmp(const WCHAR *s1, const WCHAR *s2);

/* ===== NT System Calls (via ntdll.dll through FreeDLL) ===== */

/* File I/O */
extern NTSTATUS NTAPI NtCreateFile(
    PHANDLE FileHandle, ACCESS_MASK DesiredAccess,
    POBJECT_ATTRIBUTES ObjectAttributes, PIO_STATUS_BLOCK IoStatusBlock,
    PLARGE_INTEGER AllocationSize, ULONG FileAttributes,
    ULONG ShareAccess, ULONG CreateDisposition, ULONG CreateOptions,
    PVOID EaBuffer, ULONG EaLength
);

extern NTSTATUS NTAPI NtReadFile(
    HANDLE FileHandle, HANDLE Event, PIO_APC_ROUTINE ApcRoutine,
    PVOID ApcContext, PIO_STATUS_BLOCK IoStatusBlock,
    PVOID Buffer, ULONG Length, PLARGE_INTEGER ByteOffset,
    PULONG Key
);

extern NTSTATUS NTAPI NtQueryInformationFile(
    HANDLE FileHandle,
    PIO_STATUS_BLOCK IoStatusBlock,
    PVOID FileInformation,
    ULONG Length,
    DWORD FileInformationClass
);

extern NTSTATUS NTAPI NtClose(HANDLE Handle);
extern NTSTATUS NTAPI NtAllocateVirtualMemory(
    HANDLE ProcessHandle, PVOID *BaseAddress,
    PVOID *ZeroBits, PSIZE_T RegionSize,
    ULONG AllocationType, ULONG Protect
);
extern NTSTATUS NTAPI NtProtectVirtualMemory(
    HANDLE ProcessHandle, PVOID *BaseAddress,
    PSIZE_T RegionSize, ULONG NewProtect, PULONG OldProtect
);
extern NTSTATUS NTAPI NtFreeVirtualMemory(
    HANDLE ProcessHandle, PVOID *BaseAddress,
    PSIZE_T RegionSize, ULONG FreeType
);

/* Access rights */
#define GENERIC_READ    0x80000000L
#define GENERIC_WRITE   0x40000000L
#define GENERIC_EXECUTE 0x20000000L
#define GENERIC_ALL     0x10000000L

#define FILE_SHARE_READ             0x00000001
#define FILE_SHARE_WRITE            0x00000002
#define FILE_SHARE_DELETE           0x00000004
#define FILE_OPEN                   0x00000001
#define FILE_NON_DIRECTORY_FILE     0x00000020L
#define FILE_ATTRIBUTE_NORMAL       0x00000080L

/* File information class */
#define FileStandardInformation     1

/* File standard information structure */
typedef struct _FILE_STANDARD_INFORMATION {
    LARGE_INTEGER NumberOfLinks;
    BOOLEAN DeletePending;
    BOOLEAN Directory;
    USHORT Mode;
    LARGE_INTEGER Size;
    LARGE_INTEGER IoStatusBlock;
} FILE_STANDARD_INFORMATION, *PFILE_STANDARD_INFORMATION;

/* I/O APC callback type */
typedef void (NTAPI *PIO_APC_ROUTINE)(
    PVOID ApcContext,
    PIO_STATUS_BLOCK IoStatusBlock,
    ULONG Reserved
);

#define PAGE_NOACCESS       0x01
#define PAGE_READONLY       0x02
#define PAGE_READWRITE      0x04
#define PAGE_WRITECOPY      0x08
#define PAGE_EXECUTE        0x10
#define PAGE_EXECUTE_READ   0x20
#define PAGE_EXECUTE_READWRITE  0x40

#endif /* _NTDYLIB_H */
