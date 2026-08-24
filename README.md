# FreeNT

FreeNT is an experimental, open-source **full userland replacement for
Windows NT**. It provides its own implementations of the user-mode components
that Microsoft ships in `%SystemRoot%\System32` — including the Win32
subsystem DLLs (kernel32, user32, gdi32), the session manager (smss), the
Windows subsystem (csrss/winlogon), the service control manager, and the
graphical shell (explorer) — plus a Tiny C Runtime (FreeDLL) and a PE dynamic
linker (NTDYLIB).

The userland is written in freestanding C: it has no Python runtime, C
runtime, or dependency on the proprietary Microsoft `kernel32.dll`,
`ucrtbase.dll`, or `msvcrt.dll`. All components import only `ntdll.dll` for
actual NT kernel system calls. The kernel and essential boot drivers remain
Microsoft-provided; everything above them is FreeNT.

## Build

### Prerequisites

- Linux host with `x86_64-w64-mingw32-gcc` (MinGW-w64 cross-compiler)
- GNU Make (`gmake`) — system `make` on BSD systems may not support
  `ifeq` conditionals
- PDCurses (for the WinPE TUI installer)

### Building PDCurses (one-time)

```sh
cd /tmp
git clone https://github.com/wmcbrine/PDCurses.git pdcurses
cd pdcurses/wincon
gmake CC=x86_64-w64-mingw32-gcc
ln -sf pdcurses.a libpdcurses.a   # linker expects libpdcurses.a
```

### Building all components

```sh
# 1. Build freent.exe, freedll.dll, and ntdylib.dll
gmake

# 2. Build the WinPE TUI installer (requires PDCurses)
gmake installer PDCURSES_DIR=/tmp/pdcurses

# 3. Build liberty.exe (POSIX subsystem launcher)
cd liberty && gmake
```

### Build order

1. `freedll.dll` — Tiny C Runtime + NT-compatible API (companion DLL to ntdll)
2. `ntdylib.dll` — PE dynamic linker (depends on freedll)
3. `freent.exe` — FreeNT executive shell
4. `freent_installer.exe` — WinPE TUI installer (depends on freedll, ntdylib, PDCurses)
5. `liberty.exe` — POSIX subsystem launcher (depends on freedll, ntdylib)

### Build artifacts

| Component | Output | Size |
|-----------|--------|------|
| freent.exe | `build/x64/freent.exe` | ~8 KB |
| freedll.dll | `build/x64/freedll.dll` | ~35 KB |
| ntdylib.dll | `build/x64/ntdylib.dll` | ~20 KB |
| freent_installer.exe | `build/x64/freent_installer.exe` | ~275 KB |
| liberty.exe | `build/x64/liberty.exe` | ~146 KB |

### Notes

- All Makefiles require GNU Make (`gmake`). BSD `make` does not support
  `ifeq` conditionals.
- DLLs use `-nostdlib` to avoid linking the C runtime; they import only
  `ntdll.dll`.
- The `.def` files use MinGW-compatible syntax (no `NONWAIT` keyword).

## Commands

```text
freent.exe info
freent.exe login
freent.exe version
```

`login` is the initial `logind`-style component: a terminal session manager
that reads a login name through `NtReadFile`, writes through `NtWriteFile`, and
clears the entered name after use. It deliberately does **not** accept a
password or claim to authenticate a user.

On NT, password authentication and creation of a logon token are LSA/Winlogon
responsibilities. They are not exposed as a safe, public `ntdll` Native API. A
future real login path needs a privileged LSA authentication package or an
approved broker; it must not be simulated in the console client.
