/*
 * nposix.c
 *
 * Single-file NT Native application.
 *
 * No:
 *   - Windows SDK
 *   - WDK
 *   - NativeShell NDK
 *   - windows.h
 *   - winternl.h
 *   - C runtime
 *
 * Build with MinGW-w64.
 *
 * This is a small POSIX-like layer directly over ntdll.dll.
 */

typedef unsigned char       UCHAR;
typedef unsigned short      USHORT;
typedef unsigned long       ULONG;
typedef unsigned long long  ULONGLONG;
typedef long                LONG;
typedef long long           LONGLONG;
typedef long                NTSTATUS;
typedef void               *PVOID;
typedef void               *HANDLE;
typedef unsigned long       ACCESS_MASK;
typedef int                 BOOLEAN;
typedef unsigned short      WCHAR;
typedef char                CHAR;

typedef WCHAR *PWSTR;
typedef CHAR  *PCHAR;

#define NULL ((void *)0)

#define TRUE  1
#define FALSE 0

#define NT_SUCCESS(x) ((NTSTATUS)(x) >= 0)

/* ------------------------------------------------------------------ */
/* Basic NT types                                                      */
/* ------------------------------------------------------------------ */

typedef struct _LARGE_INTEGER {
    LONGLONG QuadPart;
} LARGE_INTEGER, *PLARGE_INTEGER;

typedef struct _IO_STATUS_BLOCK {
    union {
        NTSTATUS Status;
        PVOID Pointer;
    };
    ULONGLONG Information;
} IO_STATUS_BLOCK, *PIO_STATUS_BLOCK;

typedef struct _UNICODE_STRING {
    USHORT Length;
    USHORT MaximumLength;
    PWSTR  Buffer;
} UNICODE_STRING, *PUNICODE_STRING;

typedef struct _OBJECT_ATTRIBUTES {
    ULONG Length;
    HANDLE RootDirectory;
    PUNICODE_STRING ObjectName;
    ULONG Attributes;
    PVOID SecurityDescriptor;
    PVOID SecurityQualityOfService;
} OBJECT_ATTRIBUTES, *POBJECT_ATTRIBUTES;

typedef struct _FILE_BASIC_INFORMATION {
    LARGE_INTEGER CreationTime;
    LARGE_INTEGER LastAccessTime;
    LARGE_INTEGER LastWriteTime;
    LARGE_INTEGER ChangeTime;
    ULONG FileAttributes;
} FILE_BASIC_INFORMATION;

typedef struct _FILE_STANDARD_INFORMATION {
    LARGE_INTEGER AllocationSize;
    LARGE_INTEGER EndOfFile;
    ULONG NumberOfLinks;
    BOOLEAN DeletePending;
    BOOLEAN Directory;
} FILE_STANDARD_INFORMATION;

typedef struct _FILE_POSITION_INFORMATION {
    LARGE_INTEGER CurrentByteOffset;
} FILE_POSITION_INFORMATION;

typedef struct _FILE_DISPOSITION_INFORMATION {
    BOOLEAN DeleteFile;
} FILE_DISPOSITION_INFORMATION;

/* ------------------------------------------------------------------ */
/* Native constants                                                    */
/* ------------------------------------------------------------------ */

#define STATUS_SUCCESS             ((NTSTATUS)0x00000000L)
#define STATUS_END_OF_FILE         ((NTSTATUS)0xC0000011L)
#define STATUS_OBJECT_NAME_NOT_FOUND ((NTSTATUS)0xC0000034L)
#define STATUS_NO_SUCH_FILE        ((NTSTATUS)0xC000000FL)
#define STATUS_ACCESS_DENIED       ((NTSTATUS)0xC0000022L)
#define STATUS_INVALID_PARAMETER   ((NTSTATUS)0xC000000DL)

#define OBJ_CASE_INSENSITIVE       0x00000040UL

#define FILE_READ_DATA             0x0001
#define FILE_WRITE_DATA            0x0002
#define FILE_APPEND_DATA           0x0004
#define FILE_READ_ATTRIBUTES       0x0080

#define FILE_LIST_DIRECTORY        0x0001
#define FILE_TRAVERSE              0x0020

#define SYNCHRONIZE                0x00100000

#define FILE_SHARE_READ            0x00000001
#define FILE_SHARE_WRITE           0x00000002
#define FILE_SHARE_DELETE          0x00000004

#define FILE_SUPERSEDE             0
#define FILE_OPEN                  1
#define FILE_CREATE                2
#define FILE_OPEN_IF               3
#define FILE_OVERWRITE             4
#define FILE_OVERWRITE_IF          5

#define FILE_DIRECTORY_FILE        0x00000001
#define FILE_NON_DIRECTORY_FILE    0x00000040
#define FILE_SYNCHRONOUS_IO_NONALERT 0x00000020

#define FILE_ATTRIBUTE_NORMAL      0x00000080
#define FILE_ATTRIBUTE_DIRECTORY   0x00000010

#define FileBasicInformation       4
#define FileStandardInformation   5
#define FilePositionInformation   14
#define FileDispositionInformation 13

#define DUPLICATE_SAME_ACCESS      0x00000002

#define O_RDONLY                   0
#define O_WRONLY                   1
#define O_RDWR                     2
#define O_CREAT                    0x40
#define O_TRUNC                    0x200
#define O_APPEND                   0x400

#define SEEK_SET                   0
#define SEEK_CUR                   1
#define SEEK_END                   2

#define NPOSIX_MAX_FDS             256
#define NPOSIX_MAX_PATH            1024

/* ------------------------------------------------------------------ */
/* ntdll imports                                                      */
/* ------------------------------------------------------------------ */

#define NTAPI __attribute__((stdcall))

extern NTSTATUS NTAPI NtCreateFile(
    HANDLE *FileHandle,
    ACCESS_MASK DesiredAccess,
    OBJECT_ATTRIBUTES *ObjectAttributes,
    IO_STATUS_BLOCK *IoStatusBlock,
    LARGE_INTEGER *AllocationSize,
    ULONG FileAttributes,
    ULONG ShareAccess,
    ULONG CreateDisposition,
    ULONG CreateOptions,
    PVOID EaBuffer,
    ULONG EaLength
);

extern NTSTATUS NTAPI NtReadFile(
    HANDLE FileHandle,
    HANDLE Event,
    PVOID ApcRoutine,
    PVOID ApcContext,
    IO_STATUS_BLOCK *IoStatusBlock,
    PVOID Buffer,
    ULONG Length,
    LARGE_INTEGER *ByteOffset,
    ULONG *Key
);

extern NTSTATUS NTAPI NtWriteFile(
    HANDLE FileHandle,
    HANDLE Event,
    PVOID ApcRoutine,
    PVOID ApcContext,
    IO_STATUS_BLOCK *IoStatusBlock,
    PVOID Buffer,
    ULONG Length,
    LARGE_INTEGER *ByteOffset,
    ULONG *Key
);

extern NTSTATUS NTAPI NtClose(
    HANDLE Handle
);

extern NTSTATUS NTAPI NtQueryInformationFile(
    HANDLE FileHandle,
    IO_STATUS_BLOCK *IoStatusBlock,
    PVOID FileInformation,
    ULONG Length,
    ULONG FileInformationClass
);

extern NTSTATUS NTAPI NtSetInformationFile(
    HANDLE FileHandle,
    IO_STATUS_BLOCK *IoStatusBlock,
    PVOID FileInformation,
    ULONG Length,
    ULONG FileInformationClass
);

extern NTSTATUS NTAPI NtDuplicateObject(
    HANDLE SourceProcessHandle,
    HANDLE SourceHandle,
    HANDLE TargetProcessHandle,
    HANDLE *TargetHandle,
    ACCESS_MASK DesiredAccess,
    ULONG HandleAttributes,
    ULONG Options
);

extern NTSTATUS NTAPI NtTerminateProcess(
    HANDLE ProcessHandle,
    NTSTATUS ExitStatus
);

/*
 * Native output.
 *
 * NtDisplayString is useful during early/native execution, although it
 * isn't a replacement for a normal console terminal.
 */
extern NTSTATUS NTAPI NtDisplayString(
    UNICODE_STRING *String
);

/*
 * Native startup.
 *
 * The exact startup structure differs between NT versions. We don't need
 * to consume it for this minimal implementation.
 */
typedef struct _RTL_USER_PROCESS_PARAMETERS_MIN {
    PVOID Reserved1[16];
    PVOID Reserved2[10];
} RTL_USER_PROCESS_PARAMETERS_MIN;

typedef struct _PEB_MIN {
    UCHAR Reserved1[0x20];
    PVOID ProcessParameters;
} PEB_MIN;

/* ------------------------------------------------------------------ */
/* Tiny internal memory/string routines                                */
/* ------------------------------------------------------------------ */

static void *
memcpy_local(
    void *dst,
    const void *src,
    ULONG n
)
{
    UCHAR *d = (UCHAR *)dst;
    const UCHAR *s = (const UCHAR *)src;

    while (n--)
        *d++ = *s++;

    return dst;
}

static void *
memset_local(
    void *dst,
    int c,
    ULONG n
)
{
    UCHAR *d = (UCHAR *)dst;

    while (n--)
        *d++ = (UCHAR)c;

    return dst;
}

static ULONG
strlen_local(
    const CHAR *s
)
{
    ULONG n = 0;

    while (s[n])
        n++;

    return n;
}

static ULONG
strlen_w_local(
    const WCHAR *s
)
{
    ULONG n = 0;

    while (s[n])
        n++;

    return n;
}

/* ------------------------------------------------------------------ */
/* Tiny ASCII -> Unicode conversion                                    */
/* ------------------------------------------------------------------ */

static ULONG
ascii_to_unicode(
    const CHAR *src,
    WCHAR *dst,
    ULONG max
)
{
    ULONG i = 0;

    while (src[i] && i + 1 < max)
    {
        CHAR c = src[i];

        /*
         * POSIX paths use '/' while NT object paths use '\'.
         */
        if (c == '/')
            c = '\\';

        dst[i] = (WCHAR)(unsigned char)c;
        i++;
    }

    dst[i] = 0;

    return i;
}

/* ------------------------------------------------------------------ */
/* NT path construction                                                */
/* ------------------------------------------------------------------ */

static int
make_nt_path(
    const CHAR *path,
    WCHAR *buffer,
    ULONG buffer_chars
)
{
    ULONG len;
    ULONG i;

    if (!path || !buffer)
        return -1;

    len = strlen_local(path);

    if (len + 16 >= buffer_chars)
        return -1;

    /*
     * Already a native NT path:
     *
     *     \??\C:\foo
     *     \Device\...
     */
    if (path[0] == '\\')
    {
        ascii_to_unicode(
            path,
            buffer,
            buffer_chars
        );

        return 0;
    }

    /*
     * DOS/Windows drive path:
     *
     *     C:\foo
     */
    if (len >= 2 && path[1] == ':')
    {
        buffer[0] = '\\';
        buffer[1] = '?';
        buffer[2] = '?';
        buffer[3] = '\\';

        for (i = 0; i < len && i + 5 < buffer_chars; i++)
        {
            CHAR c = path[i];

            if (c == '/')
                c = '\\';

            buffer[4 + i] =
                (WCHAR)(unsigned char)c;
        }

        buffer[4 + i] = 0;

        return 0;
    }

    /*
     * Simple POSIX root:
     *
     *     /foo/bar
     *
     * For now we map '/' to C:\.
     */
    if (path[0] == '/')
    {
        buffer[0] = '\\';
        buffer[1] = '?';
        buffer[2] = '?';
        buffer[3] = '\\';
        buffer[4] = 'C';
        buffer[5] = ':';

        for (i = 0; i < len && i + 7 < buffer_chars; i++)
        {
            CHAR c = path[i];

            if (c == '/')
                c = '\\';

            buffer[6 + i] =
                (WCHAR)(unsigned char)c;
        }

        buffer[6 + i] = 0;

        return 0;
    }

    return -1;
}

/* ------------------------------------------------------------------ */
/* Descriptor layer                                                    */
/* ------------------------------------------------------------------ */

typedef struct _NP_FD
{
    HANDLE Handle;
    ULONG Flags;
    int Used;
} NP_FD;

static NP_FD Fds[NPOSIX_MAX_FDS];

static HANDLE
fd_handle(
    int fd
)
{
    if (fd < 0 || fd >= NPOSIX_MAX_FDS)
        return NULL;

    if (!Fds[fd].Used)
        return NULL;

    return Fds[fd].Handle;
}

static int
fd_alloc(
    HANDLE handle,
    ULONG flags
)
{
    int i;

    for (i = 0; i < NPOSIX_MAX_FDS; i++)
    {
        if (!Fds[i].Used)
        {
            Fds[i].Handle = handle;
            Fds[i].Flags = flags;
            Fds[i].Used = TRUE;

            return i;
        }
    }

    return -1;
}

/* ------------------------------------------------------------------ */
/* errno-ish translation                                               */
/* ------------------------------------------------------------------ */

static int
nt_to_errno(
    NTSTATUS status
)
{
    switch ((ULONG)status)
    {
        case 0xC0000034:
        case 0xC000000F:
            return 2;       /* ENOENT */

        case 0xC0000022:
            return 13;      /* EACCES */

        case 0xC000000D:
            return 22;      /* EINVAL */

        default:
            return 5;       /* EIO */
    }
}

/* ------------------------------------------------------------------ */
/* open                                                                */
/* ------------------------------------------------------------------ */

int
nposix_open(
    const CHAR *path,
    int flags
)
{
    WCHAR path_buffer[NPOSIX_MAX_PATH];
    UNICODE_STRING name;
    OBJECT_ATTRIBUTES oa;
    IO_STATUS_BLOCK iosb;
    HANDLE handle;
    ACCESS_MASK access;
    ULONG disposition;
    ULONG options;
    NTSTATUS status;
    int fd;

    if (make_nt_path(
            path,
            path_buffer,
            NPOSIX_MAX_PATH) != 0)
        return -22;

    name.Length =
        (USHORT)(strlen_w_local(path_buffer) *
                 sizeof(WCHAR));

    name.MaximumLength =
        name.Length + sizeof(WCHAR);

    name.Buffer = path_buffer;

    memset_local(
        &oa,
        0,
        sizeof(oa)
    );

    oa.Length = sizeof(oa);
    oa.ObjectName = &name;
    oa.Attributes = OBJ_CASE_INSENSITIVE;

    access =
        SYNCHRONIZE |
        FILE_READ_ATTRIBUTES;

    switch (flags & 3)
    {
        case O_WRONLY:
            access |= FILE_WRITE_DATA;
            break;

        case O_RDWR:
            access |=
                FILE_READ_DATA |
                FILE_WRITE_DATA;
            break;

        default:
            access |= FILE_READ_DATA;
            break;
    }

    if (flags & O_APPEND)
        access |= FILE_APPEND_DATA;

    if (flags & O_TRUNC)
        disposition = FILE_OVERWRITE_IF;
    else if (flags & O_CREAT)
        disposition = FILE_OPEN_IF;
    else
        disposition = FILE_OPEN;

    options =
        FILE_NON_DIRECTORY_FILE |
        FILE_SYNCHRONOUS_IO_NONALERT;

    status = NtCreateFile(
        &handle,
        access,
        &oa,
        &iosb,
        NULL,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ |
        FILE_SHARE_WRITE |
        FILE_SHARE_DELETE,
        disposition,
        options,
        NULL,
        0
    );

    if (!NT_SUCCESS(status))
        return -nt_to_errno(status);

    fd = fd_alloc(
        handle,
        (ULONG)flags
    );

    if (fd < 0)
    {
        NtClose(handle);
        return -24;
    }

    return fd;
}

/* ------------------------------------------------------------------ */
/* close                                                               */
/* ------------------------------------------------------------------ */

int
nposix_close(
    int fd
)
{
    HANDLE handle;

    handle = fd_handle(fd);

    if (!handle)
        return -9;

    NtClose(handle);

    Fds[fd].Handle = NULL;
    Fds[fd].Flags = 0;
    Fds[fd].Used = FALSE;

    return 0;
}

/* ------------------------------------------------------------------ */
/* read                                                                */
/* ------------------------------------------------------------------ */

long
nposix_read(
    int fd,
    void *buffer,
    ULONG count
)
{
    HANDLE handle;
    IO_STATUS_BLOCK iosb;
    NTSTATUS status;

    handle = fd_handle(fd);

    if (!handle)
        return -9;

    status = NtReadFile(
        handle,
        NULL,
        NULL,
        NULL,
        &iosb,
        buffer,
        count,
        NULL,
        NULL
    );

    if ((ULONG)status == 0xC0000011)
        return 0;

    if (!NT_SUCCESS(status))
        return -nt_to_errno(status);

    return (long)iosb.Information;
}

/* ------------------------------------------------------------------ */
/* write                                                               */
/* ------------------------------------------------------------------ */

long
nposix_write(
    int fd,
    const void *buffer,
    ULONG count
)
{
    HANDLE handle;
    IO_STATUS_BLOCK iosb;
    NTSTATUS status;

    handle = fd_handle(fd);

    if (!handle)
        return -9;

    status = NtWriteFile(
        handle,
        NULL,
        NULL,
        NULL,
        &iosb,
        (PVOID)buffer,
        count,
        NULL,
        NULL
    );

    if (!NT_SUCCESS(status))
        return -nt_to_errno(status);

    return (long)iosb.Information;
}

/* ------------------------------------------------------------------ */
/* lseek and descriptor duplication                                   */
/* ------------------------------------------------------------------ */

long
nposix_lseek(int fd, long offset, int whence)
{
    HANDLE handle = fd_handle(fd);
    FILE_POSITION_INFORMATION position;
    FILE_STANDARD_INFORMATION standard;
    IO_STATUS_BLOCK iosb;
    NTSTATUS status;

    if (!handle) return -9;
    if (whence == SEEK_SET) position.CurrentByteOffset.QuadPart = offset;
    else if (whence == SEEK_CUR) {
        status = NtQueryInformationFile(handle, &iosb, &position, sizeof(position), FilePositionInformation);
        if (!NT_SUCCESS(status)) return -nt_to_errno(status);
        position.CurrentByteOffset.QuadPart += offset;
    } else if (whence == SEEK_END) {
        status = NtQueryInformationFile(handle, &iosb, &standard, sizeof(standard), FileStandardInformation);
        if (!NT_SUCCESS(status)) return -nt_to_errno(status);
        position.CurrentByteOffset.QuadPart = standard.EndOfFile.QuadPart + offset;
    } else return -22;
    if (position.CurrentByteOffset.QuadPart < 0) return -22;
    status = NtSetInformationFile(handle, &iosb, &position, sizeof(position), FilePositionInformation);
    if (!NT_SUCCESS(status)) return -nt_to_errno(status);
    return (long)position.CurrentByteOffset.QuadPart;
}

int
nposix_dup(int fd)
{
    HANDLE source = fd_handle(fd), duplicate = NULL;
    NTSTATUS status;
    int newfd;
    if (!source) return -9;
    status = NtDuplicateObject((HANDLE)(-1LL), source, (HANDLE)(-1LL),
                               &duplicate, 0, 0, DUPLICATE_SAME_ACCESS);
    if (!NT_SUCCESS(status)) return -nt_to_errno(status);
    newfd = fd_alloc(duplicate, Fds[fd].Flags);
    if (newfd < 0) { NtClose(duplicate); return -24; }
    return newfd;
}

/* ------------------------------------------------------------------ */
/* fstat                                                               */
/* ------------------------------------------------------------------ */

typedef struct _NP_STAT
{
    ULONGLONG Size;
    ULONG Attributes;
    ULONG Mode;
} NP_STAT;

int
nposix_fstat(
    int fd,
    NP_STAT *st
)
{
    HANDLE handle;
    FILE_STANDARD_INFORMATION standard;
    FILE_BASIC_INFORMATION basic;
    IO_STATUS_BLOCK iosb;
    NTSTATUS status;

    handle = fd_handle(fd);

    if (!handle || !st)
        return -22;

    status = NtQueryInformationFile(
        handle,
        &iosb,
        &standard,
        sizeof(standard),
        FileStandardInformation
    );

    if (!NT_SUCCESS(status))
        return -nt_to_errno(status);

    status = NtQueryInformationFile(
        handle,
        &iosb,
        &basic,
        sizeof(basic),
        FileBasicInformation
    );

    if (!NT_SUCCESS(status))
        return -nt_to_errno(status);

    st->Size =
        (ULONGLONG)standard.EndOfFile.QuadPart;

    st->Attributes =
        basic.FileAttributes;

    if (standard.Directory)
        st->Mode = 0040000 | 0755;
    else
        st->Mode = 0100000 | 0644;

    return 0;
}

/* ------------------------------------------------------------------ */
/* exit                                                                */
/* ------------------------------------------------------------------ */

__attribute__((noreturn))
void
nposix_exit(
    int status
)
{
    NtTerminateProcess(
        (HANDLE)(-1LL),
        (NTSTATUS)status
    );

    for (;;)
        ;
}

/* ------------------------------------------------------------------ */
/* Native entry point                                                  */
/* ------------------------------------------------------------------ */

void
NtProcessStartup(
    PVOID StartupInfo
)
{
    int fd;

    static const CHAR message[] =
        "Native POSIX layer: hello from NT Native Mode\r\n";

    (void)StartupInfo;

    fd = nposix_open(
        "\\??\\C:\\nposix-test.txt",
        O_CREAT | O_RDWR
    );

    if (fd >= 0)
    {
        nposix_write(
            fd,
            message,
            sizeof(message) - 1
        );

        nposix_close(fd);
    }

    nposix_exit(0);
}
