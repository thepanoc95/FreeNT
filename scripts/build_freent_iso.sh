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
#   - FreeNT build artifacts
set -uo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

OUTPUT_ISO="freent-installer.iso"
WORK_DIR="/tmp/freent-iso"
WINPE_ISO=""
PDCURSES_DIR="/tmp/pdcurses"

for tool in wimlib-imagex; do
    if ! command -v "$tool" &>/dev/null; then
        echo "Error: $tool not found. Please install wimlib."
        exit 1
    fi
done

if ! command -v genisofs &>/dev/null && ! command -v xorriso &>/dev/null && ! command -v genisoimage &>/dev/null; then
    echo "Error: Neither genisoimage nor xorriso found."
    exit 1
fi

while [[ $# -gt 0 ]]; do
    case "$1" in
        --output) OUTPUT_ISO="$2"; shift 2 ;;
        --pdcurses) PDCURSES_DIR="$2"; shift 2 ;;
        --workdir) WORK_DIR="$2"; shift 2 ;;
        --help|-h)
            echo "Usage: $0 [options] <winpe.iso>"
            echo "  --output <file.iso>     Output ISO path"
            echo "  --pdcurses <dir>         Path to PDCurses"
            echo "  --workdir <dir>         Working directory"
            exit 0 ;;
        -*) echo "Unknown option: $1"; exit 1 ;;
        *) WINPE_ISO="$1"; shift ;;
    esac
done

[[ -z "$WINPE_ISO" ]] && { echo "Error: No WinPE ISO specified."; exit 1; }
[[ ! -f "$WINPE_ISO" ]] && { echo "Error: WinPE ISO not found: $WINPE_ISO"; exit 1; }

echo "=== FreeNT Installer ISO Builder ==="
echo "Input WinPE ISO: $WINPE_ISO"
echo "Output ISO:      $OUTPUT_ISO"
echo "Working dir:     $WORK_DIR"

# --- Step 1: Extract WinPE ISO ---
echo "[1/6] Extracting WinPE ISO..."
if [[ -d "$WORK_DIR" ]]; then
    sudo rm -rf "$WORK_DIR" 2>/dev/null || rm -rf "$WORK_DIR" 2>/dev/null || true
fi
mkdir -p "$WORK_DIR/extract"

if command -v xorriso &>/dev/null; then
    xorriso -indev "$WINPE_ISO" -osirrox on -extract / "$WORK_DIR/extract" 2>&1 | tail -3
    sudo chmod -R u+rwX "$WORK_DIR/extract" 2>/dev/null || chmod -R u+rwX "$WORK_DIR/extract" 2>/dev/null || true
else
    MOUNT_POINT=$(mktemp -d)
    sudo mount -o loop "$WINPE_ISO" "$MOUNT_POINT" 2>/dev/null || mount -o loop "$WINPE_ISO" "$MOUNT_POINT"
    cp -a "$MOUNT_POINT/." "$WORK_DIR/extract/"
    sudo umount "$MOUNT_POINT" 2>/dev/null || umount "$MOUNT_POINT"
    rmdir "$MOUNT_POINT"
fi

# --- Step 2: Find the boot WIM ---
echo "[2/6] Locating boot.wim..."
BOOT_WIM=""
while IFS= read -r -d '' wim; do
    BOOT_WIM="$wim"
    break
done < <(find "$WORK_DIR/extract" -iname 'boot*.wim' -print0)

[[ -z "$BOOT_WIM" ]] && { echo "Error: boot.wim not found in the extracted ISO."; exit 1; }
echo "Found: $BOOT_WIM"
wimlib-imagex info "$BOOT_WIM" | head -20

# --- Step 3: Extract the WIM ---
echo "[3/7] Extracting boot.wim..."
WIM_ROOT="$WORK_DIR/wim_root"
mkdir -p "$WIM_ROOT"
wimlib-imagex apply "$BOOT_WIM" 1 "$WIM_ROOT"

# --- Step 4: Inject FreeNT components ---
echo "[4/7] Injecting FreeNT components into WinPE image..."
FREE_NT_DIR="$WIM_ROOT/Windows/System32"
mkdir -p "$FREE_NT_DIR"

if [[ -f "$PROJECT_ROOT/build/x64/freedll.dll" ]]; then
    echo "  - Copying freedll.dll"
    cp "$PROJECT_ROOT/build/x64/freedll.dll" "$FREE_NT_DIR/"
    cp "$PROJECT_ROOT/build/x64/libfreedll.a" "$FREE_NT_DIR/" 2>/dev/null || true
fi

if [[ -f "$PROJECT_ROOT/build/x64/ntdylib.dll" ]]; then
    echo "  - Copying ntdylib.dll"
    cp "$PROJECT_ROOT/build/x64/ntdylib.dll" "$FREE_NT_DIR/"
    cp "$PROJECT_ROOT/build/x64/libntdylib.a" "$FREE_NT_DIR/" 2>/dev/null || true
fi

if [[ -f "$PROJECT_ROOT/build/x64/freent.exe" ]]; then
    echo "  - Copying freent.exe"
    cp "$PROJECT_ROOT/build/x64/freent.exe" "$FREE_NT_DIR/"
fi

if [[ -f "$PROJECT_ROOT/build/x64/liberty.exe" ]]; then
    echo "  - Copying liberty.exe"
    cp "$PROJECT_ROOT/build/x64/liberty.exe" "$FREE_NT_DIR/"
fi

if [[ -f "$PROJECT_ROOT/build/x64/freent_installer.exe" ]]; then
    echo "  - Copying freent_installer.exe"
    mkdir -p "$WIM_ROOT/freent"
    cp "$PROJECT_ROOT/build/x64/freent_installer.exe" "$WIM_ROOT/freent/"
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

# Copy patches (just copy, don't apply)
PATCH_DIR="$WIM_ROOT/freent/patches"
mkdir -p "$PATCH_DIR"
for reg in osname.reg smss.reg systemroot.reg; do
    if [[ -f "$PROJECT_ROOT/patches/$reg" ]]; then
        cp "$PROJECT_ROOT/patches/$reg" "$PATCH_DIR/"
    fi
done

# Copy NativeShell files
if [[ -d "$PROJECT_ROOT/nativeshell" ]]; then
    mkdir -p "$WIM_ROOT/freent/nativeshell"
    cp -r "$PROJECT_ROOT/nativeshell/"* "$WIM_ROOT/freent/nativeshell/" 2>/dev/null || true
    if [[ -f "$PROJECT_ROOT/build/x64/native.exe" ]]; then
        cp "$PROJECT_ROOT/build/x64/native.exe" "$WIM_ROOT/freent/nativeshell/" 2>/dev/null || true
    fi
fi

# --- Step 5: Configure WinPE auto-launch ---
echo "[5/7] Configuring WinPE boot behavior..."
WINPESHL_DIR="$WIM_ROOT/Windows/System32"
mkdir -p "$WINPESHL_DIR"
cat > "$WINPESHL_DIR/winpeshl.ini" << 'EOF'
[LaunchApp]
AppPath = X:\freent\freent_installer.exe
EOF

# --- Step 6: Recapture the modified WIM ---
echo "[6/7] Recapturing boot.wim..."
NEW_WIM="$WORK_DIR/new_boot.wim"
wimlib-imagex capture "$WIM_ROOT" "$NEW_WIM" "Microsoft Windows PE (x86)" \
    --boot --compress=LZX --check 2>&1 | tail -3
sudo cp "$NEW_WIM" "$BOOT_WIM" 2>/dev/null || cp "$NEW_WIM" "$BOOT_WIM" || true
sudo rm -rf "$WIM_ROOT" 2>/dev/null || rm -rf "$WIM_ROOT" 2>/dev/null || true
sudo rm -f "$NEW_WIM" 2>/dev/null || rm -f "$NEW_WIM" 2>/dev/null || true
sudo chmod -R u+rwX "$WORK_DIR/extract" 2>/dev/null || chmod -R u+rwX "$WORK_DIR/extract" 2>/dev/null || true

# --- Step 7: Create the new ISO ---
echo "[7/7] Creating output ISO: $OUTPUT_ISO"
rm -f "$OUTPUT_ISO"

# Find boot image and compute relative path
BOOT_IMG=$(find "$WORK_DIR/extract" -iname 'etfsboot.com' -print -quit)
BOOT_REL=""
if [[ -n "$BOOT_IMG" ]]; then
    WORK_DIR_ABS=$(cd "$WORK_DIR/extract" && pwd)
    BOOT_REL="${BOOT_IMG#$WORK_DIR_ABS/}"
fi

if [[ -z "$BOOT_REL" ]]; then
    echo "Error: etfsboot.com not found in extracted ISO."
    exit 1
fi
echo "Boot image: $BOOT_REL"

if command -v xorriso &>/dev/null; then
    xorriso -as mkisofs \
        -o "$OUTPUT_ISO" \
        -b "$BOOT_REL" \
        -no-emul-boot \
        -boot-load-size 8 \
        -boot-info-table \
        -iso-level 4 \
        -joliet on \
        -volid "FREENT-INSTALLER" \
        "$WORK_DIR/extract" 2>&1 | tail -5
elif command -v genisoimage &>/dev/null; then
    genisoimage -o "$OUTPUT_ISO" \
        -b "$BOOT_REL" \
        -no-emul-boot \
        -boot-load-size 8 \
        -boot-info-table \
        -iso-level 4 \
        -J \
        -V "FREENT-INSTALLER" \
        "$WORK_DIR/extract" 2>&1 | tail -5
fi

sudo rm -rf "$WORK_DIR" 2>/dev/null || rm -rf "$WORK_DIR" 2>/dev/null || true

echo ""
echo "=== Done! ==="
echo "FreeNT Installer ISO created: $OUTPUT_ISO"
echo ""
echo "To use the installer ISO:"
echo "  1. Boot the target machine from the ISO"
echo "  2. The FreeNT installer TUI launches automatically"
echo "  3. Run: freent_installer.exe --freent-kernel"
echo "    (copies WinPE NT components directly instead of applying a WIM)"
echo ""
echo "Build components first: gmake installer PDCURSES_DIR=$PDCURSES_DIR"
