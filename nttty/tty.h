#ifndef _FREENT_TTY_H_
#define _FREENT_TTY_H_

#include <ntdef.h>
#include <ntapi.h>

/* ===== BSD TTY Subsystem for FreeNT ===== */

/* TTY line discipline constants (BSD-compatible) */
#define NTTY_COOKED     0x0
#define NTTY_RAW        0x1
#define NTTY_CBREAK     0x2

/* Control flags (c_cflag) */
#define NTTY_CS5        0x0000
#define NTTY_CS6        0x0100
#define NTTY_CS7        0x0200
#define NTTY_CS8        0x0300
#define NTTY_CSTOPB     0x0400  /* 2 stop bits */
#define NTTY_CREAD      0x0800  /* Enable receiver */
#define NTTY_PARENB     0x1000  /* Parity enable */
#define NTTY_PARODD     0x2000  /* Odd parity */
#define NTTY_HUPCL      0x4000  /* Hang up on last close */
#define NTTY_CLOCAL     0x8000  /* Ignore modem status lines */

/* Input flags (c_iflag) */
#define NTTY_IGNBRK     0x0001  /* Ignore BREAK condition */
#define NTTY_BRKINT     0x0002  /* Signal interrupt on BREAK */
#define NTTY_IGNPAR     0x0004  /* Ignore characters with parity errors */
#define NTTY_PARMRK     0x0008  /* Mark parity errors */
#define NTTY_INPCK      0x0010  /* Enable parity checking */
#define NTTY_ISTRIP     0x0020  /* Strip characters to 7-bits */
#define NTTY_INLCR      0x0040  /* Translate NL to CR */
#define NTTY_IGNCR      0x0080  /* Ignore CR */
#define NTTY_ICRNL      0x0100  /* Translate CR to NL */
#define NTTY_IXON       0x0200  /* Enable start/stop output control */
#define NTTY_IXANY      0x0400  /* Allow any char to restart output */
#define NTTY_IXOFF      0x0800  /* Enable start/stop input control */
#define NTTY_IMAXBEL    0x2000  /* Ring bell if input queue is full */

/* Output flags (c_oflag) */
#define NTTY_OPOST      0x0001  /* Post-process output */
#define NTTY_ONLCR      0x0004  /* Translate NL to CR-NL */
#define NTTY_OCRNL      0x0008  /* Translate CR to NL */
#define NTTY_OXTABS     0x0004  /* Expand tabs to spaces */

/* Local flags (c_lflag) */
#define NTTY_ISIG       0x0080  /* Enable signal processing */
#define NTTY_ICANON     0x0100  /* Canonical input (line editing) */
#define NTTY_XCASE      0x0004  /* Canonical upper/lower case */
#define NTTY_ECHO       0x0008  /* Echo input */
#define NTTY_ECHOE      0x0010  /* Echo erase as BS-SP-BS */
#define NTTY_ECHOK      0x0020  /* Echo KILL as line kill */
#define NTTY_ECHONL     0x0040  /* Echo NL even without ECHO */
#define NTTY_NOFLSH     0x8000  /* Don't flush on signal */
#define NTTY_TOSTOP     0x0400  /* Send SIGSTOP for background output */
#define NTTY_IEXTEN     0x8000  /* Enable extended input processing */

/* Special characters indices */
#define NTTY_VINTR      0
#define NTTY_VQUIT      1
#define NTTY_VERASE     2
#define NTTY_VKILL      3
#define NTTY_VEOF       4
#define NTTY_VEOL       5
#define NTTY_VEOL2      6
#define NTTY_VSTART     7
#define NTTY_VSTOP      8
#define NTTY_VSUSP      9
#define NTTY_VDSUSP     10
#define NTTY_VREPRINT   11
#define NTTY_VWERASE    12
#define NTTY_VLNEXT     13
#define NTTY_VSTATUS    14
#define NTTY_NCCS       20

/* Baud rate constants */
#define NTTY_B0         0
#define NTTY_B50        1
#define NTTY_B75        2
#define NTTY_B110       3
#define NTTY_B134       4
#define NTTY_B150       5
#define NTTY_B200       6
#define NTTY_B300       7
#define NTTY_B600       8
#define NTTY_B1200      9
#define NTTY_B1800      10
#define NTTY_B2400      11
#define NTTY_B4800      12
#define NTTY_B9600      13
#define NTTY_B19200     14
#define NTTY_B38400     15
#define NTTY_B57600     16
#define NTTY_B115200    17
#define NTTY_B230400    18

/* IOCTL commands */
#define NTTY_TCGETS     0x5401
#define NTTY_TCSETS     0x5402
#define NTTY_TCSETSW    0x5403
#define NTTY_TCSETSF    0x5404
#define NTTY_TCGETA     0x5405
#define NTTY_TCSETA     0x5406
#define NTTY_TCSETAW    0x5407
#define NTTY_TCSETAF    0x5408
#define NTTY_TCSBRK     0x5409
#define NTTY_TCXONC     0x540A
#define NTTY_TCFLSH     0x540B
#define NTTY_TIOCGWINSZ 0x5413
#define NTTY_TIOCSWINSZ 0x5414
#define NTTY_TIOCMGET   0x5415
#define NTTY_TIOCMSET   0x5418

/* Termios structure (BSD/POSIX compatible) */
typedef struct {
    ULONG c_iflag;              /* Input flags */
    ULONG c_oflag;              /* Output flags */
    ULONG c_cflag;              /* Control flags */
    ULONG c_lflag;              /* Local flags */
    UCHAR c_line;               /* Line discipline */
    UCHAR c_cc[NTTY_NCCS];      /* Special characters */
    UINT c_ispeed;              /* Input speed */
    UINT c_ospeed;              /* Output speed */
} NTTY_TERMIOS, *PNTTY_TERMIOS;

/* Window size structure */
typedef struct {
    USHORT ws_row;              /* Rows in characters */
    USHORT ws_col;              /* Columns in characters */
    USHORT ws_xpixel;           /* Horizontal size in pixels */
    USHORT ws_ypixel;           /* Vertical size in pixels */
} NTTY_WINSIZE, *PNTTY_WINSIZE;

/* TTY device state flags */
typedef struct {
    ULONG flags;
    ULONG state;
} NTTY_FLAGS, *PNTTY_FLAGS;

#define NTTY_OPEN       0x00000001
#define NTTY_ISCTTY     0x00000002  /* Is controlling terminal */
#define NTTY_EXCLUSIVE  0x00000004
#define NTTY_BLOCKED    0x00000008
#define NTTY_ASLEEP     0x00000010
#define NTTY_XCLUDE     0x00000020
#define NTTY_HUPCLS     0x00000040

/* TTY input buffer */
#define NTTY_IBUF_SIZE  1024

typedef struct {
    UCHAR buf[NTTY_IBUF_SIZE];
    ULONG head;
    ULONG tail;
    ULONG count;
    HANDLE lock;
} NTTY_IBUF, *PNTTY_IBUF;

/* TTY output buffer */
#define NTTY_OBUF_SIZE  4096

typedef struct {
    UCHAR buf[NTTY_OBUF_SIZE];
    ULONG head;
    ULONG tail;
    ULONG count;
    HANDLE lock;
} NTTY_OBUF, *PNTTY_OBUF;

/* Line discipline table */
typedef struct {
    NTSTATUS (*open)(PVOID tty);
    NTSTATUS (*close)(PVOID tty);
    NTSTATUS (*read)(PVOID tty, PVOID buf, ULONG len, PULONG read_len);
    NTSTATUS (*write)(PVOID tty, PVOID buf, ULONG len, PULONG written_len);
    NTSTATUS (*ioctl)(PVOID tty, ULONG cmd, PVOID data);
    NTSTATUS (*input)(PVOID tty, UCHAR ch);
    NTSTATUS (*output)(PVOID tty, UCHAR ch);
    NTSTATUS (*modem_ctrl)(PVOID tty, ULONG cmd, ULONG *status);
} NTTY_LINE_DISCIPLINE, *PNTTY_LINE_DISCIPLINE;

/* Main TTY device structure */
typedef struct _NTTY_DEVICE {
    /* Basic device info */
    ULONG id;
    UCHAR type;                 /* Master/slave/PTY */
    UCHAR subtype;              /* Console, serial, pseudo */
    WCHAR device_name[32];
    HANDLE device_handle;
    HANDLE process_handle;
    ULONG owner_pid;
    ULONG owner_uid;
    ULONG owner_gid;
    ULONG mode;

    /* Terminal settings */
    NTTY_TERMIOS termios;
    NTTY_WINSIZE winsize;
    NTTY_FLAGS tty_flags;

    /* Buffers */
    NTTY_IBUF input_buf;
    NTTY_OBUF output_buf;

    /* Line discipline */
    PNTTY_LINE_DISCIPLINE line_disc;

    /* Process group management */
    ULONG foreground_pgid;
    ULONG session_leader_pid;
    HANDLE session_leader_handle;

    /* Callbacks */
    NTSTATUS (*on_input_available)(struct _NTTY_DEVICE *);
    NTSTATUS (*on_output_complete)(struct _NTTY_DEVICE *);
    NTSTATUS (*on_break)(struct _NTTY_DEVICE *);

    /* Statistics */
    ULARGE_INTEGER bytes_read;
    ULARGE_INTEGER bytes_written;
    ULONG input_errors;
    ULONG output_errors;

    /* Reference count */
    LONG ref_count;
    
    /* Synchronization */
    HANDLE input_ready_event;
    HANDLE output_ready_event;
    HANDLE control_lock;
    struct _NTTY_DEVICE *next;

} NTTY_DEVICE, *PNTTY_DEVICE;

/* ===== Function Declarations ===== */

/* Core TTY Management */
NTSTATUS
NttyInitialize(VOID);

NTSTATUS
NttyShutdown(VOID);

NTSTATUS
NttyCreateDevice(
    PWCHAR DeviceName,
    UCHAR DeviceType,
    PNTTY_DEVICE *OutDevice
);

NTSTATUS
NttyCloseDevice(
    PNTTY_DEVICE Device
);

NTSTATUS
NttyOpenDevice(
    PWCHAR DeviceName,
    ACCESS_MASK DesiredAccess,
    PNTTY_DEVICE *OutDevice
);

/* Input/Output Operations */
NTSTATUS
NttyRead(
    PNTTY_DEVICE Device,
    PVOID Buffer,
    ULONG Length,
    PULONG BytesRead
);

NTSTATUS
NttyWrite(
    PNTTY_DEVICE Device,
    PVOID Buffer,
    ULONG Length,
    PULONG BytesWritten
);

NTSTATUS
NttyGetChar(
    PNTTY_DEVICE Device,
    PUCHAR Char
);

NTSTATUS
NttyPutChar(
    PNTTY_DEVICE Device,
    UCHAR Char
);

/* Terminal Control */
NTSTATUS
NttyGetTermios(
    PNTTY_DEVICE Device,
    PNTTY_TERMIOS Termios
);

NTSTATUS
NttySetTermios(
    PNTTY_DEVICE Device,
    PNTTY_TERMIOS Termios
);

NTSTATUS
NttyGetWinsize(
    PNTTY_DEVICE Device,
    PNTTY_WINSIZE Winsize
);

NTSTATUS
NttySetWinsize(
    PNTTY_DEVICE Device,
    PNTTY_WINSIZE Winsize
);

NTSTATUS
NttyIoctl(
    PNTTY_DEVICE Device,
    ULONG ControlCode,
    PVOID InputBuffer,
    ULONG InputLength,
    PVOID OutputBuffer,
    ULONG OutputLength,
    PULONG BytesReturned
);

/* Signal/Control Operations */
NTSTATUS
NttySendSignal(
    PNTTY_DEVICE Device,
    UCHAR Signal
);

NTSTATUS
NttyHandleBreak(
    PNTTY_DEVICE Device
);

NTSTATUS
NttyFlush(
    PNTTY_DEVICE Device,
    ULONG QueueSelector
);

/* Line Discipline Management */
NTSTATUS
NttySetLineDisc(
    PNTTY_DEVICE Device,
    UCHAR LineDisc
);

NTSTATUS
NttyGetLineDisc(
    PNTTY_DEVICE Device,
    PUCHAR LineDisc
);

/* Process Group Management */
NTSTATUS
NttySetForegroundGroup(
    PNTTY_DEVICE Device,
    ULONG ProcessGroupId
);

NTSTATUS
NttyGetForegroundGroup(
    PNTTY_DEVICE Device,
    PULONG ProcessGroupId
);

/* Buffer Management */
NTSTATUS
NttyFlushInputBuffer(
    PNTTY_DEVICE Device
);

NTSTATUS
NttyFlushOutputBuffer(
    PNTTY_DEVICE Device
);

NTSTATUS
NttyGetInputCount(
    PNTTY_DEVICE Device,
    PULONG Count
);

NTSTATUS
NttyGetOutputCount(
    PNTTY_DEVICE Device,
    PULONG Count
);

/* Mode/Discipline Functions */
NTSTATUS
NttySetMode(
    PNTTY_DEVICE Device,
    UCHAR Mode
);

NTSTATUS
NttyGetMode(
    PNTTY_DEVICE Device,
    PUCHAR Mode
);

/* Statistics */
NTSTATUS
NttyGetStats(
    PNTTY_DEVICE Device,
    PULARGE_INTEGER BytesRead,
    PULARGE_INTEGER BytesWritten,
    PULONG InputErrors,
    PULONG OutputErrors
);

#endif /* _FREENT_TTY_H_ */