# FreeNT

FreeNT is an experimental, open-source **full userland replacement for
Windows NT**. It provides its own implementations of the user-mode components
that Microsoft ships in `%SystemRoot%\System32` — including the Win32
subsystem DLLs (kernel32, user32, gdi32), the session manager (smss), the
Windows subsystem (csrss/winlogon), the service control manager, and the
graphical shell (explorer) — plus a Tiny C Runtime (FreeDLL) and a PE dynamic
linker (NTDYLIB).

The userland is written in freestanding C: it has no C
runtime, or dependency on the proprietary Microsoft `kernel32.dll`,
`ucrtbase.dll`, or `msvcrt.dll`. All components import only `ntdll.dll` for
actual NT kernel system calls. The kernel and essential boot drivers remain
Microsoft-provided; everything above them is FreeNT.

## Native compatibility components

- **FreeDLL** now exposes additional ANSI C allocation, search, pseudo-random,
  and string-set primitives for freestanding Native Mode programs.
- **NTVM** provides a FreeDLL-backed real-mode `.COM` loader and CPU context
  contract, forming the portable host boundary for an NTVDM-style interpreter.
- **NTTTY** supplies BSD/POSIX-like termios state, canonical/raw line
  disciplines, terminal buffers, and process-group controls for Native Mode.
