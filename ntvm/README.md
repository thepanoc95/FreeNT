# NTVM

NTVM is the FreeNT DOS application host.  It follows the architectural split
of [NTVDM](https://github.com/davidly/ntvdm): an x86 real-mode core is kept
separate from the NT host services.  Unlike the original Windows component,
the host is freestanding and imports the C runtime from `freedll.dll`, rather
than a Microsoft CRT or `kernel32.dll`.

The initial implementation deliberately supports the useful and testable
loader portion: DOS `.COM` images are placed at offset `0x100` in a 64 KiB
real-mode segment, with a correctly initialized CS:IP, DS, ES, SS:SP register
state.  The interpreter/backend is an explicit next step; no DOS program is
silently executed as native code.

## ABI contract

`ntvm.h` contains an embeddable CPU state and a loader API.  The host that
reads a DOS executable owns file I/O; this keeps the emulator independent of
a Win32 subsystem and lets Native Mode applications provide file handles via
NT syscalls.  `NtvmLoadCom` validates the image and creates a complete initial
real-mode context.  `NtvmDestroy` releases its FreeDLL-backed memory.

Build with `make -f ntvm/Makefile` after building `freedll.dll`.
