#include <ntdef.h>
#include <ntapi.h>
#include "tty.h"

/* Forward declarations */
extern NTSTATUS RtlCopyMemory(PVOID Dest, PVOID Src, SIZE_T Size);
extern NTSTATUS RtlZeroMemory(PVOID Dest, SIZE_T Size);
extern NTSTATUS RtlMoveMemory(PVOID Dest, PVOID Src, SIZE_T Size);
extern NTSTATUS RtlFillMemory(PVOID Dest, SIZE_T Size, UCHAR Value);
extern HANDLE GetProcessHeap(VOID);
extern PVOID RtlAllocateHeap(HANDLE HeapHandle, ULONG Flags, SIZE_T Size);
extern BOOLEAN RtlFreeHeap(HANDLE HeapHandle, ULONG Flags, PVOID Ptr);
extern HANDLE CreateHeap(ULONG Flags, SIZE_T InitialSize, SIZE_T MaxSize);
extern NTSTATUS GetCurrentProcessId(VOID);
extern NTSTATUS GetCurrentThreadId(VOID);
extern NTSTATUS Sleep(ULONG Milliseconds);
extern NTSTATUS RtlNtStatusToDosError(NTSTATUS Status);
extern VOID RtlCopyUnicodeString(PUNICODE_STRING Dest, PUNICODE_STRING Src);

/* Global TTY subsystem state */
typedef struct {
    HANDLE heap_handle;
    ULONG device_count;
    PNTTY_DEVICE device_list;
    HANDLE list_lock;
    BOOLEAN initialized;
} NTTY_SUBSYSTEM, *PNTTY_SUBSYSTEM;

static NTTY_SUBSYSTEM g_ntty_subsystem = {0};

/* ===== Helper Functions ===== */

static VOID
NttyAcquireLock(PHANDLE Lock)
{
    if (!*Lock) {
        NtCreateMutant(Lock, MUTANT_ALL_ACCESS, NULL, FALSE);
    }
}

static VOID
NttyReleaseLock(HANDLE Lock)
{
    if (Lock) {
        NtReleaseMutant(Lock, NULL);
    }
}

static BOOLEAN
NttyIsCarriageReturn(UCHAR Ch)
{
    return Ch == '\r';
}

static BOOLEAN
NttyIsLineFeed(UCHAR Ch)
{
    return Ch == '\n';
}

static BOOLEAN
NttyIsControl(UCHAR Ch)
{
    return Ch < 0x20 || Ch == 0x7F;
}

static VOID
NttyInitTermios(PNTTY_TERMIOS Termios)
{
    if (!Termios) return;

    RtlZeroMemory(Termios, sizeof(NTTY_TERMIOS));

    /* Default input flags: ICRNL, IXON enabled */
    Termios->c_iflag = NTTY_ICRNL | NTTY_IXON | NTTY_IXANY;

    /* Default output flags: OPOST enabled */
    Termios->c_oflag = NTTY_OPOST | NTTY_ONLCR;

    /* Default control flags: 8-bit, 1 stop bit, no parity */
    Termios->c_cflag = NTTY_CS8 | NTTY_CREAD | NTTY_CLOCAL;

    /* Default local flags: ISIG, ICANON, ECHO all enabled */
    Termios->c_lflag = NTTY_ISIG | NTTY_ICANON | NTTY_ECHO | 
                       NTTY_ECHOE | NTTY_ECHOK | NTTY_ECHONL;

    /* Default line discipline: cooked mode */
    Termios->c_line = NTTY_COOKED;

    /* Default speeds: 9600 baud */
    Termios->c_ispeed = NTTY_B9600;
    Termios->c_ospeed = NTTY_B9600;

    /* Special characters */
    Termios->c_cc[NTTY_VINTR]   = 0x03;  /* ^C */
    Termios->c_cc[NTTY_VQUIT]   = 0x1C;  /* ^\ */
    Termios->c_cc[NTTY_VERASE]  = 0x08;  /* ^H (backspace) */
    Termios->c_cc[NTTY_VKILL]   = 0x15;  /* ^U */
    Termios->c_cc[NTTY_VEOF]    = 0x04;  /* ^D */
    Termios->c_cc[NTTY_VEOL]    = 0x00;  /* None */
    Termios->c_cc[NTTY_VEOL2]   = 0x00;  /* None */
    Termios->c_cc[NTTY_VSTART]  = 0x11;  /* ^Q */
    Termios->c_cc[NTTY_VSTOP]   = 0x13;  /* ^S */
    Termios->c_cc[NTTY_VSUSP]   = 0x1A;  /* ^Z */
    Termios->c_cc[NTTY_VDSUSP]  = 0x19;  /* ^Y */
    Termios->c_cc[NTTY_VREPRINT]= 0x12;  /* ^R */
    Termios->c_cc[NTTY_VWERASE] = 0x17;  /* ^W */
    Termios->c_cc[NTTY_VLNEXT]  = 0x16;  /* ^V */
    Termios->c_cc[NTTY_VSTATUS] = 0x14;  /* ^T */
}

static VOID
NttyInitWinsize(PNTTY_WINSIZE Winsize)
{
    if (!Winsize) return;

    RtlZeroMemory(Winsize, sizeof(NTTY_WINSIZE));
    Winsize->ws_row = 25;
    Winsize->ws_col = 80;
    Winsize->ws_xpixel = 640;
    Winsize->ws_ypixel = 400;
}

/* ===== Input Buffer Management ===== */

static NTSTATUS
NttyInitInputBuffer(PNTTY_IBUF IBuf)
{
    if (!IBuf) return STATUS_INVALID_PARAMETER;

    RtlZeroMemory(IBuf, sizeof(NTTY_IBUF));
    IBuf->head = 0;
    IBuf->tail = 0;
    IBuf->count = 0;

    return NtCreateMutant(&IBuf->lock, MUTANT_ALL_ACCESS, NULL, FALSE);
}

static NTSTATUS
NttyPushInputChar(PNTTY_IBUF IBuf, UCHAR Ch)
{
    if (!IBuf) return STATUS_INVALID_PARAMETER;
    if (IBuf->count >= NTTY_IBUF_SIZE) return STATUS_BUFFER_OVERFLOW;

    NtlyAcquireLock(&IBuf->lock);

    IBuf->buf[IBuf->tail] = Ch;
    IBuf->tail = (IBuf->tail + 1) % NTTY_IBUF_SIZE;
    IBuf->count++;

    NttyReleaseLock(IBuf->lock);

    return STATUS_SUCCESS;
}

static NTSTATUS
NttyPopInputChar(PNTTY_IBUF IBuf, PUCHAR Ch)
{
    if (!IBuf || !Ch) return STATUS_INVALID_PARAMETER;
    if (IBuf->count == 0) return STATUS_NO_DATA_DETECTED;

    NttyAcquireLock(&IBuf->lock);

    *Ch = IBuf->buf[IBuf->head];
    IBuf->head = (IBuf->head + 1) % NTTY_IBUF_SIZE;
    IBuf->count--;

    NttyReleaseLock(IBuf->lock);

    return STATUS_SUCCESS;
}

/* ===== Output Buffer Management ===== */

static NTSTATUS
NttyInitOutputBuffer(PNTTY_OBUF OBuf)
{
    if (!OBuf) return STATUS_INVALID_PARAMETER;

    RtlZeroMemory(OBuf, sizeof(NTTY_OBUF));
    OBuf->head = 0;
    OBuf->tail = 0;
    OBuf->count = 0;

    return NtCreateMutant(&OBuf->lock, MUTANT_ALL_ACCESS, NULL, FALSE);
}

static NTSTATUS
NttyPushOutputChar(PNTTY_OBUF OBuf, UCHAR Ch)
{
    if (!OBuf) return STATUS_INVALID_PARAMETER;
    if (OBuf->count >= NTTY_OBUF_SIZE) return STATUS_BUFFER_OVERFLOW;

    NttyAcquireLock(&OBuf->lock);

    OBuf->buf[OBuf->tail] = Ch;
    OBuf->tail = (OBuf->tail + 1) % NTTY_OBUF_SIZE;
    OBuf->count++;

    NttyReleaseLock(OBuf->lock);

    return STATUS_SUCCESS;
}

static NTSTATUS
NttyPopOutputChar(PNTTY_OBUF OBuf, PUCHAR Ch)
{
    if (!OBuf || !Ch) return STATUS_INVALID_PARAMETER;
    if (OBuf->count == 0) return STATUS_NO_DATA_DETECTED;

    NttyAcquireLock(&OBuf->lock);

    *Ch = OBuf->buf[OBuf->head];
    OBuf->head = (OBuf->head + 1) % NTTY_OBUF_SIZE;
    OBuf->count--;

    NttyReleaseLock(OBuf->lock);

    return STATUS_SUCCESS;
}

/* ===== Line Discipline: Cooked Mode ===== */

static NTSTATUS
NttyCookedModeOpen(PVOID TtyPtr)
{
    PNTTY_DEVICE Tty = (PNTTY_DEVICE)TtyPtr;
    if (!Tty) return STATUS_INVALID_PARAMETER;

    Tty->tty_flags.flags |= NTTY_OPEN;
    return STATUS_SUCCESS;
}

static NTSTATUS
NttyCookedModeClose(PVOID TtyPtr)
{
    PNTTY_DEVICE Tty = (PNTTY_DEVICE)TtyPtr;
    if (!Tty) return STATUS_INVALID_PARAMETER;

    Tty->tty_flags.flags &= ~NTTY_OPEN;
    return STATUS_SUCCESS;
}

static NTSTATUS
NttyCookedModeRead(PVOID TtyPtr, PVOID Buf, ULONG Len, PULONG ReadLen)
{
    PNTTY_DEVICE Tty = (PNTTY_DEVICE)TtyPtr;
    PUCHAR Buffer = (PUCHAR)Buf;
    ULONG i = 0;
    UCHAR ch;
    NTSTATUS Status;

    if (!Tty || !Buf || !ReadLen) return STATUS_INVALID_PARAMETER;

    *ReadLen = 0;

    while (i < Len) {
        Status = NttyPopInputChar(&Tty->input_buf, &ch);
        if (!NT_SUCCESS(Status)) break;

        Buffer[i++] = ch;
    }

    *ReadLen = i;
    return i > 0 ? STATUS_SUCCESS : Status;
}

static NTSTATUS
NttyCookedModeWrite(PVOID TtyPtr, PVOID Buf, ULONG Len, PULONG WrittenLen)
{
    PNTTY_DEVICE Tty = (PNTTY_DEVICE)TtyPtr;
    PUCHAR Buffer = (PUCHAR)Buf;
    ULONG i;

    if (!Tty || !Buf || !WrittenLen) return STATUS_INVALID_PARAMETER;

    for (i = 0; i < Len; i++) {
        NTSTATUS Status = NttyPushOutputChar(&Tty->output_buf, Buffer[i]);
        if (!NT_SUCCESS(Status)) break;
    }

    *WrittenLen = i;
    Tty->bytes_written.QuadPart += i;

    return STATUS_SUCCESS;
}

static NTSTATUS
NttyCookedModeInput(PVOID TtyPtr, UCHAR Ch)
{
    PNTTY_DEVICE Tty = (PNTTY_DEVICE)TtyPtr;
    PNTTY_TERMIOS T;

    if (!Tty) return STATUS_INVALID_PARAMETER;

    T = &Tty->termios;

    /* Check for special characters */
    if ((T->c_lflag & NTTY_ISIG) && Ch == T->c_cc[NTTY_VINTR]) {
        /* Send SIGINT */
        return NttySendSignal(Tty, NTTY_VINTR);
    }

    if ((T->c_lflag & NTTY_ISIG) && Ch == T->c_cc[NTTY_VQUIT]) {
        /* Send SIGQUIT */
        return NttySendSignal(Tty, NTTY_VQUIT);
    }

    if ((T->c_lflag & NTTY_ISIG) && Ch == T->c_cc[NTTY_VSUSP]) {
        /* Send SIGSUSP */
        return NttySendSignal(Tty, NTTY_VSUSP);
    }

    /* Handle erase character */
    if ((T->c_lflag & NTTY_ICANON) && Ch == T->c_cc[NTTY_VERASE]) {
        if (Tty->input_buf.count > 0) {
            Tty->input_buf.count--;
            if (T->c_lflag & NTTY_ECHOE) {
                /* Echo backspace-space-backspace */
                NttyPushOutputChar(&Tty->output_buf, '\b');
                NttyPushOutputChar(&Tty->output_buf, ' ');
                NttyPushOutputChar(&Tty->output_buf, '\b');
            }
        }
        return STATUS_SUCCESS;
    }

    /* Handle kill character */
    if ((T->c_lflag & NTTY_ICANON) && Ch == T->c_cc[NTTY_VKILL]) {
        Tty->input_buf.count = 0;
        Tty->input_buf.head = 0;
        Tty->input_buf.tail = 0;
        if (T->c_lflag & NTTY_ECHOK) {
            NttyPushOutputChar(&Tty->output_buf, '\n');
        }
        return STATUS_SUCCESS;
    }

    /* Handle EOF character */
    if ((T->c_lflag & NTTY_ICANON) && Ch == T->c_cc[NTTY_VEOF]) {
        /* Signal EOF by returning empty read */
        return STATUS_END_OF_FILE;
    }

    /* Input translation */
    if (T->c_iflag & NTTY_ICRNL) {
        if (Ch == '\r') Ch = '\n';
    }

    if (T->c_iflag & NTTY_INLCR) {
        if (Ch == '\n') Ch = '\r';
    }

    /* Push to input buffer */
    NttyPushInputChar(&Tty->input_buf, Ch);

    /* Echo handling */
    if (T->c_lflag & NTTY_ECHO) {
        NttyPushOutputChar(&Tty->output_buf, Ch);
    }

    return STATUS_SUCCESS;
}

static NTSTATUS
NttyCookedModeOutput(PVOID TtyPtr, UCHAR Ch)
{
    PNTTY_DEVICE Tty = (PNTTY_DEVICE)TtyPtr;

    if (!Tty) return STATUS_INVALID_PARAMETER;

    /* Output translation */
    if (Tty->termios.c_oflag & NTTY_OPOST) {
        if (Ch == '\n' && (Tty->termios.c_oflag & NTTY_ONLCR)) {
            NttyPushOutputChar(&Tty->output_buf, '\r');
            NttyPushOutputChar(&Tty->output_buf, '\n');
            return STATUS_SUCCESS;
        }
    }

    return NttyPushOutputChar(&Tty->output_buf, Ch);
}

static NTSTATUS
NttyCookedModeIoctl(PVOID TtyPtr, ULONG Cmd, PVOID Data)
{
    PNTTY_DEVICE Tty = (PNTTY_DEVICE)TtyPtr;

    if (!Tty) return STATUS_INVALID_PARAMETER;

    switch (Cmd) {
    case NTTY_TCGETS:
        if (Data) {
            RtlCopyMemory(Data, &Tty->termios, sizeof(NTTY_TERMIOS));
            return STATUS_SUCCESS;
        }
        break;

    case NTTY_TCSETS:
    case NTTY_TCSETSW:
    case NTTY_TCSETSF:
        if (Data) {
            RtlCopyMemory(&Tty->termios, Data, sizeof(NTTY_TERMIOS));
            return STATUS_SUCCESS;
        }
        break;

    case NTTY_TIOCGWINSZ:
        if (Data) {
            RtlCopyMemory(Data, &Tty->winsize, sizeof(NTTY_WINSIZE));
            return STATUS_SUCCESS;
        }
        break;

    case NTTY_TIOCSWINSZ:
        if (Data) {
            RtlCopyMemory(&Tty->winsize, Data, sizeof(NTTY_WINSIZE));
            return STATUS_SUCCESS;
        }
        break;

    case NTTY_TCFLSH:
        if (Data) {
            PULONG Selector = (PULONG)Data;
            if (*Selector == 0) NttyFlushInputBuffer(Tty);
            if (*Selector == 1) NttyFlushOutputBuffer(Tty);
            if (*Selector == 2) {
                NttyFlushInputBuffer(Tty);
                NttyFlushOutputBuffer(Tty);
            }
            return STATUS_SUCCESS;
        }
        break;
    }

    return STATUS_NOT_SUPPORTED;
}

static NTSTATUS
NttyCookedModeModemCtrl(PVOID TtyPtr, ULONG Cmd, ULONG *Status)
{
    /* Not implemented for console TTY */
    return STATUS_NOT_SUPPORTED;
}

/* Cooked mode line discipline */
static NTTY_LINE_DISCIPLINE g_ntty_cooked_disc = {
    NttyCookedModeOpen,
    NttyCookedModeClose,
    NttyCookedModeRead,
    NttyCookedModeWrite,
    NttyCookedModeIoctl,
    NttyCookedModeInput,
    NttyCookedModeOutput,
    NttyCookedModeModemCtrl
};

/* ===== Line Discipline: Raw Mode ===== */

static NTSTATUS
NttyRawModeRead(PVOID TtyPtr, PVOID Buf, ULONG Len, PULONG ReadLen)
{
    PNTTY_DEVICE Tty = (PNTTY_DEVICE)TtyPtr;
    PUCHAR Buffer = (PUCHAR)Buf;
    ULONG i = 0;
    UCHAR ch;
    NTSTATUS Status;

    if (!Tty || !Buf || !ReadLen) return STATUS_INVALID_PARAMETER;

    *ReadLen = 0;

    /* Raw mode: no buffering, direct input */
    while (i < Len) {
        Status = NttyPopInputChar(&Tty->input_buf, &ch);
        if (!NT_SUCCESS(Status)) break;
        Buffer[i++] = ch;
    }

    *ReadLen = i;
    return i > 0 ? STATUS_SUCCESS : Status;
}

static NTSTATUS
NttyRawModeInput(PVOID TtyPtr, UCHAR Ch)
{
    PNTTY_DEVICE Tty = (PNTTY_DEVICE)TtyPtr;

    if (!Tty) return STATUS_INVALID_PARAMETER;

    /* Raw mode: no processing, just buffer */
    return NttyPushInputChar(&Tty->input_buf, Ch);
}

static NTSTATUS
NttyRawModeOpen(PVOID TtyPtr)
{
    PNTTY_DEVICE Tty = (PNTTY_DEVICE)TtyPtr;
    if (!Tty) return STATUS_INVALID_PARAMETER;

    Tty->tty_flags.flags |= NTTY_OPEN;
    Tty->termios.c_lflag &= ~(NTTY_ICANON | NTTY_ECHO);
    return STATUS_SUCCESS;
}

static NTSTATUS
NttyRawModeClose(PVOID TtyPtr)
{
    return NttyCookedModeClose(TtyPtr);
}

static NTSTATUS
NttyRawModeWrite(PVOID TtyPtr, PVOID Buf, ULONG Len, PULONG WrittenLen)
{
    return NttyCookedModeWrite(TtyPtr, Buf, Len, WrittenLen);
}

static NTSTATUS
NttyRawModeIoctl(PVOID TtyPtr, ULONG Cmd, PVOID Data)
{
    return NttyCookedModeIoctl(TtyPtr, Cmd, Data);
}

static NTSTATUS
NttyRawModeOutput(PVOID TtyPtr, UCHAR Ch)
{
    PNTTY_DEVICE Tty = (PNTTY_DEVICE)TtyPtr;
    if (!Tty) return STATUS_INVALID_PARAMETER;
    return NttyPushOutputChar(&Tty->output_buf, Ch);
}

static NTSTATUS
NttyRawModeModemCtrl(PVOID TtyPtr, ULONG Cmd, ULONG *Status)
{
    return STATUS_NOT_SUPPORTED;
}

static NTTY_LINE_DISCIPLINE g_ntty_raw_disc = {
    NttyRawModeOpen,
    NttyRawModeClose,
    NttyRawModeRead,
    NttyRawModeWrite,
    NttyRawModeIoctl,
    NttyRawModeInput,
    NttyRawModeOutput,
    NttyRawModeModemCtrl
};

/* ===== Core TTY Functions ===== */

NTSTATUS
NttyInitialize(VOID)
{
    if (g_ntty_subsystem.initialized) return STATUS_SUCCESS;

    g_ntty_subsystem.heap_handle = GetProcessHeap();
    if (!g_ntty_subsystem.heap_handle) {
        return STATUS_UNSUCCESSFUL;
    }

    g_ntty_subsystem.device_count = 0;
    g_ntty_subsystem.device_list = NULL;

    NtCreateMutant(&g_ntty_subsystem.list_lock, 
                   MUTANT_ALL_ACCESS, NULL, FALSE);

    g_ntty_subsystem.initialized = TRUE;
    return STATUS_SUCCESS;
}

NTSTATUS
NttyShutdown(VOID)
{
    PNTTY_DEVICE Device, Next;

    if (!g_ntty_subsystem.initialized) return STATUS_SUCCESS;

    NttyAcquireLock(&g_ntty_subsystem.list_lock);

    Device = g_ntty_subsystem.device_list;
    while (Device) {
        Next = (PNTTY_DEVICE)Device + 1;  /* Assuming linked list or array */
        NttyCloseDevice(Device);
        Device = Next;
    }

    NttyReleaseLock(g_ntty_subsystem.list_lock);

    if (g_ntty_subsystem.heap_handle) {
        /* Heap cleanup handled by kernel */
    }

    g_ntty_subsystem.initialized = FALSE;
    return STATUS_SUCCESS;
}

NTSTATUS
NttyCreateDevice(
    PWCHAR DeviceName,
    UCHAR DeviceType,
    PNTTY_DEVICE *OutDevice
)
{
    PNTTY_DEVICE Device;
    NTSTATUS Status;

    if (!OutDevice) return STATUS_INVALID_PARAMETER;
    if (!g_ntty_subsystem.initialized) return STATUS_NOT_INITIALIZED;

    Device = (PNTTY_DEVICE)RtlAllocateHeap(
        g_ntty_subsystem.heap_handle, 0, sizeof(NTTY_DEVICE));

    if (!Device) return STATUS_NO_MEMORY;

    RtlZeroMemory(Device, sizeof(NTTY_DEVICE));

    Device->id = g_ntty_subsystem.device_count++;
    Device->type = DeviceType;
    Device->ref_count = 1;

    if (DeviceName) {
        /* Copy device name */
        UNICODE_STRING Name;
        RtlInitUnicodeString(&Name, DeviceName);
        /* Copy to Device->device_name */
    }

    /* Initialize termios */
    NttyInitTermios(&Device->termios);

    /* Initialize winsize */
    NttyInitWinsize(&Device->winsize);

    /* Initialize buffers */
    Status = NttyInitInputBuffer(&Device->input_buf);
    if (!NT_SUCCESS(Status)) {
        RtlFreeHeap(g_ntty_subsystem.heap_handle, 0, Device);
        return Status;
    }

    Status = NttyInitOutputBuffer(&Device->output_buf);
    if (!NT_SUCCESS(Status)) {
        RtlFreeHeap(g_ntty_subsystem.heap_handle, 0, Device);
        return Status;
    }

    /* Set default line discipline */
    Device->line_disc = &g_ntty_cooked_disc;

    /* Create synchronization events */
    Status = NtCreateEvent(&Device->input_ready_event, 
                          EVENT_ALL_ACCESS, NULL, 
                          NotificationEvent, FALSE);
    if (!NT_SUCCESS(Status)) {
        RtlFreeHeap(g_ntty_subsystem.heap_handle, 0, Device);
        return Status;
    }

    Status = NtCreateEvent(&Device->output_ready_event,
                          EVENT_ALL_ACCESS, NULL,
                          NotificationEvent, FALSE);
    if (!NT_SUCCESS(Status)) {
        RtlFreeHeap(g_ntty_subsystem.heap_handle, 0, Device);
        return Status;
    }

    *OutDevice = Device;
    return STATUS_SUCCESS;
}

NTSTATUS
NttyCloseDevice(
    PNTTY_DEVICE Device
)
{
    if (!Device) return STATUS_INVALID_PARAMETER;

    InterlockedDecrement(&Device->ref_count);

    if (Device->ref_count <= 0) {
        if (Device->device_handle) {
            NtClose(Device->device_handle);
        }
        if (Device->input_ready_event) {
            NtClose(Device->input_ready_event);
        }
        if (Device->output_ready_event) {
            NtClose(Device->output_ready_event);
        }
        if (Device->control_lock) {
            NtClose(Device->control_lock);
        }
        if (Device->input_buf.lock) {
            NtClose(Device->input_buf.lock);
        }
        if (Device->output_buf.lock) {
            NtClose(Device->output_buf.lock);
        }

        RtlFreeHeap(g_ntty_subsystem.heap_handle, 0, Device);
    }

    return STATUS_SUCCESS;
}

/* ===== I/O Operations ===== */

NTSTATUS
NttyRead(
    PNTTY_DEVICE Device,
    PVOID Buffer,
    ULONG Length,
    PULONG BytesRead
)
{
    if (!Device || !Buffer || !BytesRead) return STATUS_INVALID_PARAMETER;
    if (!Device->line_disc || !Device->line_disc->read) return STATUS_NOT_SUPPORTED;

    return Device->line_disc->read(Device, Buffer, Length, BytesRead);
}

NTSTATUS
NttyWrite(
    PNTTY_DEVICE Device,
    PVOID Buffer,
    ULONG Length,
    PULONG BytesWritten
)
{
    if (!Device || !Buffer || !BytesWritten) return STATUS_INVALID_PARAMETER;
    if (!Device->line_disc || !Device->line_disc->write) return STATUS_NOT_SUPPORTED;

    return Device->line_disc->write(Device, Buffer, Length, BytesWritten);
}

NTSTATUS
NttyGetChar(
    PNTTY_DEVICE Device,
    PUCHAR Char
)
{
    if (!Device || !Char) return STATUS_INVALID_PARAMETER;

    return NttyPopInputChar(&Device->input_buf, Char);
}

NTSTATUS
NttyPutChar(
    PNTTY_DEVICE Device,
    UCHAR Char
)
{
    if (!Device) return STATUS_INVALID_PARAMETER;

    if (Device->line_disc && Device->line_disc->output) {
        return Device->line_disc->output(Device, Char);
    }

    return NttyPushOutputChar(&Device->output_buf, Char);
}

/* ===== Terminal Control ===== */

NTSTATUS
NttyGetTermios(
    PNTTY_DEVICE Device,
    PNTTY_TERMIOS Termios
)
{
    if (!Device || !Termios) return STATUS_INVALID_PARAMETER;

    RtlCopyMemory(Termios, &Device->termios, sizeof(NTTY_TERMIOS));
    return STATUS_SUCCESS;
}

NTSTATUS
NttySetTermios(
    PNTTY_DEVICE Device,
    PNTTY_TERMIOS Termios
)
{
    if (!Device || !Termios) return STATUS_INVALID_PARAMETER;

    RtlCopyMemory(&Device->termios, Termios, sizeof(NTTY_TERMIOS));
    return STATUS_SUCCESS;
}

NTSTATUS
NttyGetWinsize(
    PNTTY_DEVICE Device,
    PNTTY_WINSIZE Winsize
)
{
    if (!Device || !Winsize) return STATUS_INVALID_PARAMETER;

    RtlCopyMemory(Winsize, &Device->winsize, sizeof(NTTY_WINSIZE));
    return STATUS_SUCCESS;
}

NTSTATUS
NttySetWinsize(
    PNTTY_DEVICE Device,
    PNTTY_WINSIZE Winsize
)
{
    if (!Device || !Winsize) return STATUS_INVALID_PARAMETER;

    RtlCopyMemory(&Device->winsize, Winsize, sizeof(NTTY_WINSIZE));
    return STATUS_SUCCESS;
}

NTSTATUS
NttyIoctl(
    PNTTY_DEVICE Device,
    ULONG ControlCode,
    PVOID InputBuffer,
    ULONG InputLength,
    PVOID OutputBuffer,
    ULONG OutputLength,
    PULONG BytesReturned
)
{
    if (!Device) return STATUS_INVALID_PARAMETER;
    if (!Device->line_disc || !Device->line_disc->ioctl) return STATUS_NOT_SUPPORTED;

    return Device->line_disc->ioctl(Device, ControlCode, InputBuffer);
}

/* ===== Signal/Control Operations ===== */

NTSTATUS
NttySendSignal(
    PNTTY_DEVICE Device,
    UCHAR Signal
)
{
    if (!Device) return STATUS_INVALID_PARAMETER;

    /* Signal delivery not implemented in this basic version */
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NttyHandleBreak(
    PNTTY_DEVICE Device
)
{
    if (!Device) return STATUS_INVALID_PARAMETER;

    if (Device->line_disc && Device->line_disc->input) {
        /* Send BREAK character */
        return Device->line_disc->input(Device, 0);
    }

    return STATUS_SUCCESS;
}

NTSTATUS
NttyFlush(
    PNTTY_DEVICE Device,
    ULONG QueueSelector
)
{
    if (!Device) return STATUS_INVALID_PARAMETER;

    if (QueueSelector == 0) {
        return NttyFlushInputBuffer(Device);
    } else if (QueueSelector == 1) {
        return NttyFlushOutputBuffer(Device);
    } else {
        NttyFlushInputBuffer(Device);
        return NttyFlushOutputBuffer(Device);
    }
}

/* ===== Line Discipline Management ===== */

NTSTATUS
NttySetLineDisc(
    PNTTY_DEVICE Device,
    UCHAR LineDisc
)
{
    if (!Device) return STATUS_INVALID_PARAMETER;

    switch (LineDisc) {
    case NTTY_COOKED:
        Device->line_disc = &g_ntty_cooked_disc;
        break;
    case NTTY_RAW:
        Device->line_disc = &g_ntty_raw_disc;
        break;
    case NTTY_CBREAK:
        /* CBREAK not implemented, use cooked */
        Device->line_disc = &g_ntty_cooked_disc;
        break;
    default:
        return STATUS_INVALID_PARAMETER;
    }

    Device->termios.c_line = LineDisc;
    return STATUS_SUCCESS;
}

NTSTATUS
NttyGetLineDisc(
    PNTTY_DEVICE Device,
    PUCHAR LineDisc
)
{
    if (!Device || !LineDisc) return STATUS_INVALID_PARAMETER;

    *LineDisc = Device->termios.c_line;
    return STATUS_SUCCESS;
}

/* ===== Process Group Management ===== */

NTSTATUS
NttySetForegroundGroup(
    PNTTY_DEVICE Device,
    ULONG ProcessGroupId
)
{
    if (!Device) return STATUS_INVALID_PARAMETER;

    Device->foreground_pgid = ProcessGroupId;
    return STATUS_SUCCESS;
}

NTSTATUS
NttyGetForegroundGroup(
    PNTTY_DEVICE Device,
    PULONG ProcessGroupId
)
{
    if (!Device || !ProcessGroupId) return STATUS_INVALID_PARAMETER;

    *ProcessGroupId = Device->foreground_pgid;
    return STATUS_SUCCESS;
}

/* ===== Buffer Management ===== */

NTSTATUS
NttyFlushInputBuffer(
    PNTTY_DEVICE Device
)
{
    if (!Device) return STATUS_INVALID_PARAMETER;

    NttyAcquireLock(&Device->input_buf.lock);
    Device->input_buf.head = 0;
    Device->input_buf.tail = 0;
    Device->input_buf.count = 0;
    NttyReleaseLock(Device->input_buf.lock);

    return STATUS_SUCCESS;
}

NTSTATUS
NttyFlushOutputBuffer(
    PNTTY_DEVICE Device
)
{
    if (!Device) return STATUS_INVALID_PARAMETER;

    NttyAcquireLock(&Device->output_buf.lock);
    Device->output_buf.head = 0;
    Device->output_buf.tail = 0;
    Device->output_buf.count = 0;
    NttyReleaseLock(Device->output_buf.lock);

    return STATUS_SUCCESS;
}

NTSTATUS
NttyGetInputCount(
    PNTTY_DEVICE Device,
    PULONG Count
)
{
    if (!Device || !Count) return STATUS_INVALID_PARAMETER;

    *Count = Device->input_buf.count;
    return STATUS_SUCCESS;
}

NTSTATUS
NttyGetOutputCount(
    PNTTY_DEVICE Device,
    PULONG Count
)
{
    if (!Device || !Count) return STATUS_INVALID_PARAMETER;

    *Count = Device->output_buf.count;
    return STATUS_SUCCESS;
}

/* ===== Mode/Discipline Functions ===== */

NTSTATUS
NttySetMode(
    PNTTY_DEVICE Device,
    UCHAR Mode
)
{
    return NttySetLineDisc(Device, Mode);
}

NTSTATUS
NttyGetMode(
    PNTTY_DEVICE Device,
    PUCHAR Mode
)
{
    return NttyGetLineDisc(Device, Mode);
}

/* ===== Statistics ===== */

NTSTATUS
NttyGetStats(
    PNTTY_DEVICE Device,
    PULARGE_INTEGER BytesRead,
    PULARGE_INTEGER BytesWritten,
    PULONG InputErrors,
    PULONG OutputErrors
)
{
    if (!Device) return STATUS_INVALID_PARAMETER;

    if (BytesRead) {
        BytesRead->QuadPart = Device->bytes_read.QuadPart;
    }
    if (BytesWritten) {
        BytesWritten->QuadPart = Device->bytes_written.QuadPart;
    }
    if (InputErrors) {
        *InputErrors = Device->input_errors;
    }
    if (OutputErrors) {
        *OutputErrors = Device->output_errors;
    }

    return STATUS_SUCCESS;
}