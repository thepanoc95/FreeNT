#!/usr/local/bin/bash
#
# build_freent_iso.sh - Create a FreeNT installer ISO from a WinPE ISO
#
# Usage: ./scripts/build_freent_iso.sh [options] <winpe.iso>
#
# Options:
#   --output <file.iso>     Output ISO path (default: freent-installer.iso)
#   --pdcurses <dir>        Path to PDCurses (default: /tmp/pdcurses)
#   --workdir <dir>         Working directory (default: /tmp/freent-iso)
#   --help                  Show this help message
#
# Prerequisites:
#   - wimlib (wimlib-imagex)
#   - genisoimage or xorriso (for ISO creation)
#   - FreeNT build artifacts (freedll.dll, ntdylib.dll, freent.exe,
#     freent_installer.exe, liberty.exe, native.exe)
#
# Prerequisites check
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

# Defaults
OUTPUT_ISO="freent-installer.iso"
WORK_DIR="/tmp/freent-iso"
WINPE_ISO=""
PDCURSES_DIR="/tmp/pdcurses"

# Check for required tools
for tool in wiminfo wimlib-imagex; do
    if ! command -v "$tool" &>/dev/null; then
        echo "Error: $tool not found. Please install wimlib."
        exit 1
    fi
done

if ! command -v genisoimage &>/dev/null && ! command -v xorriso &>/dev/null; then
    echo "Error: Neither genisoimage nor xorriso found. Please install one."
    exit 1
fi

# Parse arguments
while [[ $# -gt 0 ]]; do
    case "$1" in
        --output)
            OUTPUT_ISO="$2"
            shift 2
            ;;
        --pdcurses)
            PDCURSES_DIR="$2"
            shift 2
            ;;
        --workdir)
            WORK_DIR="$2"
            shift 2
            ;;
        --help|-h)
            echo "Usage: $0 [options] <winpe.iso>"
            echo ""
            echo "Options:"
            echo "  --output <file.iso>     Output ISO path (default: $OUTPUT_ISO)"
            echo "  --pdcurses <dir>        Path to PDCurses (default: $PDCURSES_DIR)"
            echo "  --workdir <dir>         Working directory (default: $WORK_DIR)"
            echo "  --help, -h              Show this help message"
            exit 0
            ;;
        -*)
            echo "Unknown option: $1"
            exit 1
            ;;
        *)
            WINPE_ISO="$1"
            shift
            ;;
    esac
done

if [[ -z "$WINPE_ISO" ]]; then
    echo "Error: No WinPE ISO specified."
    echo "Usage: $0 [options] <winpe.iso>"
    exit 1
fi

if [[ ! -f "$WINPE_ISO" ]]; then
    echo "Error: WinPE ISO not found: $WINPE_ISO"
    exit 1
fi

echo "=== FreeNT Installer ISO Builder ==="
echo "Input WinPE ISO: $WINPE_ISO"
echo "Output ISO:      $OUTPUT_ISO"
echo "Working dir:     $WORK_DIR"
echo ""

# --- Step 1: Extract WinPE ISO ---
echo "[1/7] Extracting WinPE ISO..."
# Clean up any previous run (may need sudo for root-owned files)
if [[ -d "$WORK_DIR" ]]; then
    rm -rf "$WORK_DIR" 2>/dev/null || sudo rm -rf "$WORK_DIR" 2>/dev/null || true
fi
mkdir -p "$WORK_DIR/extract"

if command -v xorriso &>/dev/null; then
    xorriso -indev "$WINPE_ISO" -osirrox on -extract / "$WORK_DIR/extract" 2>&1 | tail -3
    # xorriso may create root-owned files; fix permissions for subsequent steps
    chmod -R u+rwX "$WORK_DIR/extract" 2>/dev/null || sudo chmod -R u+rwX "$WORK_DIR/extract" 2>/dev/null || true
else
    # Fallback: mount and copy
    MOUNT_POINT=$(mktemp -d)
    sudo mount -o loop "$WINPE_ISO" "$MOUNT_POINT" 2>/dev/null || mount -o loop "$WINPE_ISO" "$MOUNT_POINT"
    cp -a "$MOUNT_POINT/." "$WORK_DIR/extract/"
    sudo umount "$MOUNT_POINT" 2>/dev/null || umount "$MOUNT_POINT"
    rmdir "$MOUNT_POINT"
fi

# Extract the El-Torito boot image (needed for ISO recreation)
# Some ISOs (e.g., WinPE, Vista PE) use a hidden El-Torito boot image not present
# as a file in the filesystem. We extract it using xorriso's El-Torito parser.
BOOT_IMG="$WORK_DIR/boot.img"
if command -v xorriso &>/dev/null; then
    ELTORITO_INFO=$(xorriso -indev "$WINPE_ISO" -report_el_torito plain 2>/dev/null)
    BOOT_LBA=$(echo "$ELTORITO_INFO" | grep 'El Torito boot img' | awk '{print $NF}')
    BOOT_SECTORS=$(echo "$ELTORITO_INFO" | grep 'El Torito boot img' | awk '{print $(NF-1)}')
    if [[ -n "$BOOT_LBA" && -n "$BOOT_SECTORS" ]]; then
        echo "Extracting El-Torito boot image (LBA=$BOOT_LBA, sectors=$BOOT_SECTORS)..."
        dd if="$WINPE_ISO" of="$BOOT_IMG" bs=2048 skip="$BOOT_LBA" count="$BOOT_SECTORS" 2>/dev/null
    fi
fi

# --- Step 2: Find and extract the boot WIM ---
echo "[2/7] Locating boot.wim..."
BOOT_WIM=""

# Search for WIM files (case-insensitive, since WinPE ISOs may use
# uppercase SOURCES/BOOT.WIM while we run on a case-sensitive filesystem)
while IFS= read -r -d '' wim; do
    BOOT_WIM="$wim"
    break
done < <(find "$WORK_DIR/extract" -iname 'boot*.wim' -print0)

if [[ -z "$BOOT_WIM" ]]; then
    echo "Error: boot.wim not found in the extracted ISO."
    echo "Searched in: $WORK_DIR/extract/sources/ and similar locations"
    exit 1
fi

echo "Found: $BOOT_WIM"

# List images in the WIM
echo "WIM contents:"
wimlib-imagex info "$BOOT_WIM" | head -20

# --- Step 3: Extract the WIM to a working directory ---
# Note: wimlib-imagex mount requires FUSE support which may not be compiled in.
# We use apply (extract) + capture (re-create) instead.
echo "[3/7] Extracting boot.wim..."
WIM_ROOT="$WORK_DIR/wim_root"
mkdir -p "$WIM_ROOT"
wimlib-imagex apply "$BOOT_WIM" 1 "$WIM_ROOT"

# Ensure we clean up on exit
trap 'rm -rf "$WORK_DIR" 2>/dev/null || true' EXIT

# --- Step 4: Copy FreeNT components into the WinPE image ---
echo "[4/7] Injecting FreeNT components into WinPE image..."

# Create the FreeNT directory in the target
FREE_NT_DIR="$WIM_ROOT/Windows/System32"
mkdir -p "$FREE_NT_DIR"

# Copy FreeDLL
if [[ -f "$PROJECT_ROOT/build/x64/freedll.dll" ]]; then
    echo "  - Copying freedll.dll"
    cp "$PROJECT_ROOT/build/x64/freedll.dll" "$FREE_NT_DIR/"
    cp "$PROJECT_ROOT/build/x64/libfreedll.a" "$FREE_NT_DIR/" 2>/dev/null || true
fi

# Copy NTDYLIB
if [[ -f "$PROJECT_ROOT/build/x64/ntdylib.dll" ]]; then
    echo "  - Copying ntdylib.dll"
    cp "$PROJECT_ROOT/build/x64/ntdylib.dll" "$FREE_NT_DIR/"
    cp "$PROJECT_ROOT/build/x64/libntdylib.a" "$FREE_NT_DIR/" 2>/dev/null || true
fi

# Copy FreeNT shell (freent.exe)
if [[ -f "$PROJECT_ROOT/build/x64/freent.exe" ]]; then
    echo "  - Copying freent.exe"
    cp "$PROJECT_ROOT/build/x64/freent.exe" "$FREE_NT_DIR/"
fi

# Copy Liberty (POSIX subsystem launcher)
if [[ -f "$PROJECT_ROOT/build/x64/liberty.exe" ]]; then
    echo "  - Copying liberty.exe"
    cp "$PROJECT_ROOT/build/x64/liberty.exe" "$FREE_NT_DIR/"
fi

# Copy the installer executable
if [[ -f "$PROJECT_ROOT/build/x64/freent_installer.exe" ]]; then
    echo "  - Copying freent_installer.exe"
    mkdir -p "$WIM_ROOT/freent"
    cp "$PROJECT_ROOT/build/x64/freent_installer.exe" "$WIM_ROOT/freent/"
    # Copy FreeNT components next to the installer
    cp "$PROJECT_ROOT/build/x64/freedll.dll" "$WIM_ROOT/freent/" 2>/dev/null || true
    cp "$PROJECT_ROOT/build/x64/ntdylib.dll" "$WIM_ROOT/freent/" 2>/dev/null || true
    cp "$PROJECT_ROOT/build/x64/liberty.exe" "$WIM_ROOT/freent/" 2>/dev/null || true
fi

# Copy NativeShell (native.exe)
if [[ -f "$PROJECT_ROOT/build/x64/native.exe" ]]; then
    echo "  - Copying native.exe (NativeShell)"
    cp "$PROJECT_ROOT/build/x64/native.exe" "$FREE_NT_DIR/"
elif [[ -f "$WIM_ROOT/native.exe" ]]; then
    echo "  - native.exe already present in image"
fi

# Copy patches
PATCH_DIR="$WIM_ROOT/freent/patches"
mkdir -p "$PATCH_DIR"
for reg in osname.reg smss.reg systemroot.reg; do
    if [[ -f "$PROJECT_ROOT/patches/$reg" ]]; then
        cp "$PROJECT_ROOT/patches/$reg" "$PATCH_DIR/"
    fi
done

# Copy NativeShell install files
if [[ -d "$PROJECT_ROOT/nativeshell/install" ]]; then
    mkdir -p "$WIM_ROOT/freent/nativeshell/install"
    cp "$PROJECT_ROOT/nativeshell/install/"*.reg "$WIM_ROOT/freent/nativeshell/install/" 2>/dev/null || true
fi

# --- Step 5: Configure WinPE to auto-launch the FreeNT installer ---
echo "[5/7] Configuring WinPE boot behavior..."

# Set the default shell to freent.exe in WinPE
# (WinPE uses X:\System32\winpeshl.exe which reads winpeshl.ini)
WINPESHL_DIR="$WIM_ROOT/Windows/System32"
mkdir -p "$WINPESHL_DIR"
cat > "$WINPESHL_DIR/winpeshl.ini" << 'EOF'
[LaunchApp]
AppPath = X:\freent\freent_installer.exe
EOF

# Copy the freent directory to the ISO extract root (for ISO-level access)
if [[ -d "$WIM_ROOT/freent" ]]; then
    cp -r "$WIM_ROOT/freent" "$WORK_DIR/extract/freent"
fi

# --- Step 6: Recapture the modified WIM ---
echo "[6/7] Recapturing boot.wim..."
# wimlib-imagex capture needs to write to a new file; since the original
# WIM may be root-owned (extracted by xorriso), we capture to a temp file
# and then replace the original.
NEW_WIM="$WORK_DIR/new_boot.wim"
wimlib-imagex capture "$WIM_ROOT" "$NEW_WIM" "Microsoft Windows PE (x86)" \
    --boot --compress=LZX --check 2>&1 | tail -3

# Replace the original WIM with the new one
cp "$NEW_WIM" "$BOOT_WIM" 2>/dev/null || sudo cp "$NEW_WIM" "$BOOT_WIM"

# Clean up extracted WIM root (large directory)
rm -rf "$WIM_ROOT"
rm -f "$NEW_WIM"

# --- Step 7: Create the new ISO ---
echo "[7/7] Creating output ISO: $OUTPUT_ISO"

# Clean up previous output
rm -f "$OUTPUT_ISO"

# Create the new ISO from the extracted directory with the El-Torito
# boot image. This approach recreates the ISO with proper boot structures
# using the extracted boot image (typically ETFSBOOT.COM-based) and
# -boot-info-table for BIOS boot support.
if command -v xorriso &>/dev/null; then
    # Copy the El-Torito boot image into the extract directory
    # (the boot image was extracted in Step 1 at $BOOT_IMG)
    BOOT_IMG_REL=""
    if [[ -f "$BOOT_IMG" ]]; then
        BOOT_IMG_REL=$(basename "$BOOT_IMG")
        cp "$BOOT_IMG" "$WORK_DIR/extract/$BOOT_IMG_REL"
    fi

    # Create the new ISO from the extracted directory, preserving the
# El-Torito boot image. For standard Windows PE ISOs (Win7 PE, etc.),
# the boot image is based on ETFSBOOT.COM which supports -boot-info-table.
    if [[ -n "$BOOT_IMG_REL" ]]; then
        xorriso -as mkisofs \
            -o "$OUTPUT_ISO" \
            -b "$BOOT_IMG_REL" \
            -no-emul-boot \
            -boot-load-size 4 \
            -boot-info-table \
            -iso-level 3 \
            -joliet \
            -volid "FREENTINST" \
            "$WORK_DIR/extract"
    else
        # No boot image was extracted (xorriso El-Torito parsing failed).
        # Try using the visible ETFSBOOT.COM file from the ISO.
        ETFSBOOT=$(find "$WORK_DIR/extract" -iname 'etfsboot.com' -print -quit 2>/dev/null)
        if [[ -n "$ETFSBOOT" ]]; then
            ETFSBOOT_REL=${ETFSBOOT#$WORK_DIR/extract/}
            # ETFSBOOT.COM is a standard boot image that supports -boot-info-table
            xorriso -as mkisofs \
                -o "$OUTPUT_ISO" \
                -b "$ETFSBOOT_REL" \
                -no-emul-boot \
                -boot-load-size 4 \
                -boot-info-table \
                -iso-level 3 \
                -joliet \
                -volid "FREENTINST" \
                "$WORK_DIR/extract"
        else
            echo "Error: No boot image found (ETFSBOOT.COM or El-Torito boot image)."
            exit 1
        fi
    fi
elif command -v genisoimage &>/dev/null; then
    # Fallback: create ISO from extracted directory
    BOOT_IMG_REL=""
    if [[ -f "$BOOT_IMG" ]]; then
        BOOT_IMG_REL=$(basename "$BOOT_IMG")
    fi
    if [[ -n "$BOOT_IMG_REL" ]]; then
        cp "$BOOT_IMG" "$WORK_DIR/extract/$BOOT_IMG_REL"
        genisoimage -o "$OUTPUT_ISO" \
            -b "$BOOT_IMG_REL" \
            -no-emul-boot \
            -boot-load-size 4 \
            -boot-info-table \
            -iso-level 3 \
            -J \
            -volid "FREENTINST" \
            "$WORK_DIR/extract"
    else
        echo "Error: No boot image found (etfsboot.com or El-Torito boot image)."
        exit 1
    fi
fi

echo "FreeNT installer ISO created: $OUTPUT_ISO"
