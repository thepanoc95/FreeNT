# FreeNT TUI Installer

A ncurses-style (via PDCurses) TUI installer for FreeNT that runs inside a WinPE environment.

## Overview

This installer provides a text-based user interface (TUI) for installing FreeNT onto a target disk. It runs inside a Windows PE (WinPE) live ISO environment and guides the user through the installation process interactively.

Unlike the old batch-based installer, this C-based TUI provides:
- Interactive disk selection with navigation
- UEFI/BIOS partition style selection
- WIM image path scanning and configuration
- Visual progress feedback during installation
- Registry patch application to the target system
- Boot configuration for the target system

## Build Requirements

- **PDCurses** library (for the TUI)
- **MinGW-w64 cross-compiler** (`x86_64-w64-mingw32-gcc`)
- **PDCurses** compiled for Windows (MinGW or vcpkg)

### Installing PDCurses

**Option 1: Build from source**
```bash
git clone https://github.com/PDCurses/PDCurses.git
cd PDCurses/wincon
make
```

**Option 2: Using vcpkg**
```bash
vcpkg install pdcurses
```

## Building

### Building with MinGW-w64 (from Linux)

```bash
# Install MinGW-w64 cross-compiler and PDCurses
# Debian/Ubuntu: apt install mingw-w64
# Build PDCurses for wincon:
#   cd PDCurses/wincon && make
#   Or use vcpkg: vcpkg install pdcurses

# Build the installer
make installer PDCURSES_DIR=/path/to/pdcurses

# Or specify a custom compiler
make installer PDCURSES_DIR=/path/to/pdcurses INSTALLER_CC=x86_64-w64-mingw32-gcc
```

### Building with MinGW-w64 (from Windows)

```bash
# From the installer directory
mingw32-make
```

## Running

The installer is designed to run from the FreeNT WinPE live ISO environment. The WinPE runtime runs from `X:` while the target disk is mounted as `C:` during installation.

```cmd
freent_installer.exe
```

### Command-line Options

```
freent_installer.exe [--disk N] [--uefi|--bios] [--wim PATH] [--index N]
```

## Installation Steps

The installer performs the following steps:

1. **Disk Selection** — Lists all available disks and lets you pick the target
2. **Partition Style** — Choose UEFI (GPT) or BIOS (MBR)
3. **Image Configuration** — Specify the WIM file and image index
4. **Summary** — Review the configuration before proceeding
5. **Partitioning** — DiskPart partitions and formats the target disk
6. **Image Apply** — DISM applies the Windows base image to `C:\`
7. **Post-Processing** — Renames `C:\Windows` to `C:\WNT`
8. **Cleanup** — Removes proprietary Microsoft components
9. **Registry Patches** — Applies FreeNT patches from `patches/*.reg`
10. **Boot Configuration** — Writes BCD entries or MBR boot code

## Registry Patches

The installer applies three registry patches located in `../patches/`:

- **`osname.reg`** — Sets `CurrentVersion` to `FreeNT`
- **`smss.reg`** — Configures `liberty.exe` as the session manager
- **`systemroot.reg`** — Sets `SystemRoot` to `WNT`

These patches are applied to the target system's `SYSTEM` registry hive via `reg load`/`reg unload`.

## Architecture

The installer is a standalone Windows application that uses the standard Win32 API (available in WinPE) and PDCurses for the TUI. It does **not** depend on FreeNT's freestanding libraries (FreeDLL, ntdylib) since it runs in the WinPE environment, which has the full Windows API available.

## License

BSD 3-Clause License — see `../LICENSE` for details.
