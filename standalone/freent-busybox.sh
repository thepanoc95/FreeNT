#!/bin/sh
# FreeNT BusyBox Compatible Launcher
# Copyright (c) 2026, Panoc95
# BSD 3-Clause License

# This launcher is specifically designed for BusyBox environments
# It avoids features that might not be available in minimal BusyBox installations

# Set standalone directory - BusyBox compatible
STANDALONE_DIR="$0"
# Resolve symlinks manually for BusyBox
while [ -L "$STANDALONE_DIR" ]; do
    # BusyBox readlink might not support -f, so we do it manually
    TARGET=$(readlink "$STANDALONE_DIR" 2>/dev/null || echo "")
    if [ -z "$TARGET" ]; then
        break
    fi
    # Handle relative symlinks
    case "$TARGET" in
        /*) STANDALONE_DIR="$TARGET" ;;
        *) STANDALONE_DIR="$(dirname "$STANDALONE_DIR")/$TARGET" ;;
    esac
done
STANDALONE_DIR=$(dirname "$STANDALONE_DIR")

# Function to check if command exists - BusyBox compatible
check_cmd() {
    # Method 1: Use command if available
    if command -v "$1" >/dev/null 2>&1; then
        return 0
    fi
    
    # Method 2: Use which if available
    if which "$1" >/dev/null 2>&1; then
        return 0
    fi
    
    # Method 3: Manual PATH search
    OLD_IFS="$IFS"
    IFS=":"
    for p in $PATH; do
        if [ -n "$p" ] && [ -x "$p/$1" ]; then
            IFS="$OLD_IFS"
            return 0
        fi
    done
    IFS="$OLD_IFS"
    return 1
}

# Function to find python - BusyBox compatible
find_python() {
    # Try python
    if check_cmd python; then
        echo python
        return 0
    fi
    
    # Try python3
    if check_cmd python3; then
        echo python3
        return 0
    fi
    
    # Check for portable python in standalone directory
    if [ -x "$STANDALONE_DIR/python" ]; then
        echo "$STANDALONE_DIR/python"
        return 0
    fi
    
    # BusyBox might not have -x test, so we check differently
    if [ -f "$STANDALONE_DIR/python" ] && [ -r "$STANDALONE_DIR/python" ]; then
        echo "$STANDALONE_DIR/python"
        return 0
    fi
    
    if [ -f "$STANDALONE_DIR/python/python" ] && [ -r "$STANDALONE_DIR/python/python" ]; then
        echo "$STANDALONE_DIR/python/python"
        return 0
    fi
    
    # Check parent directory
    if [ -x "$STANDALONE_DIR/../python" ]; then
        echo "$STANDALONE_DIR/../python"
        return 0
    fi
    
    if [ -f "$STANDALONE_DIR/../python" ] && [ -r "$STANDALONE_DIR/../python" ]; then
        echo "$STANDALONE_DIR/../python"
        return 0
    fi
    
    if [ -x "$STANDALONE_DIR/../python/python" ]; then
        echo "$STANDALONE_DIR/../python/python"
        return 0
    fi
    
    if [ -f "$STANDALONE_DIR/../python/python" ] && [ -r "$STANDALONE_DIR/../python/python" ]; then
        echo "$STANDALONE_DIR/../python/python"
        return 0
    fi
    
    return 1
}

# Function to check if we're running under BusyBox
is_busybox() {
    # Check if /bin/sh is BusyBox
    if [ -x /bin/busybox ]; then
        return 0
    fi
    if /bin/sh --help 2>&1 | grep -q "BusyBox"; then
        return 0
    fi
    if [ -L /bin/sh ] && readlink /bin/sh 2>/dev/null | grep -q "busybox"; then
        return 0
    fi
    return 1
}

# Detect BusyBox and show message
if is_busybox; then
    echo "Running under BusyBox environment"
fi

# Find python
PYTHON=$(find_python)

if [ -z "$PYTHON" ]; then
    echo "Error: Python is required to run FreeNT"
    echo "Please install Python or place a portable Python in the standalone directory"
    exit 1
fi

# Set PYTHONPATH if needed for portable Python
if echo "$PYTHON" | grep -q "$STANDALONE_DIR"; then
    # We're using portable Python, set PYTHONPATH
    if [ -d "$STANDALONE_DIR/../src" ]; then
        export PYTHONPATH="$STANDALONE_DIR/../src:$STANDALONE_DIR/../standalone/lib:$PYTHONPATH"
    fi
fi

# Run the login manager
"$PYTHON" "$STANDALONE_DIR/../src/login_manager/login_app.py" "$@"
