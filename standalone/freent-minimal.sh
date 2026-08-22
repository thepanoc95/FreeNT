#!/bin/sh
# FreeNT Minimal Shell Launcher
# Copyright (c) 2026, Panoc95
# BSD 3-Clause License

# This is a minimal version of the launcher that works with very basic shells
# including BusyBox ash, dash, and other minimal POSIX-compatible shells

# Set standalone directory
STANDALONE_DIR="$0"
while [ -L "$STANDALONE_DIR" ]; do
    STANDALONE_DIR=$(readlink "$STANDALONE_DIR")
done
STANDALONE_DIR=$(dirname "$STANDALONE_DIR")

# Function to check if command exists (minimal version)
check_cmd() {
    # Try command -v first (POSIX)
    if command -v "$1" >/dev/null 2>&1; then
        return 0
    fi
    # Fallback to which
    if which "$1" >/dev/null 2>&1; then
        return 0
    fi
    # Fallback to checking PATH directly
    OLD_IFS="$IFS"
    IFS=":"
    for p in $PATH; do
        if [ -x "$p/$1" ]; then
            IFS="$OLD_IFS"
            return 0
        fi
    done
    IFS="$OLD_IFS"
    return 1
}

# Function to find python (minimal version)
find_python() {
    # Try python first
    if check_cmd python; then
        echo python
        return 0
    fi
    
    # Try python3
    if check_cmd python3; then
        echo python3
        return 0
    fi
    
    # Check portable python in standalone directory
    if [ -x "$STANDALONE_DIR/python" ]; then
        echo "$STANDALONE_DIR/python"
        return 0
    fi
    
    if [ -x "$STANDALONE_DIR/python/python" ]; then
        echo "$STANDALONE_DIR/python/python"
        return 0
    fi
    
    # Check parent directory
    if [ -x "$STANDALONE_DIR/../python" ]; then
        echo "$STANDALONE_DIR/../python"
        return 0
    fi
    
    if [ -x "$STANDALONE_DIR/../python/python" ]; then
        echo "$STANDALONE_DIR/../python/python"
        return 0
    fi
    
    return 1
}

# Find python
PYTHON=$(find_python)

if [ -z "$PYTHON" ]; then
    echo "Error: Python is required to run FreeNT"
    echo "Please install Python or place a portable Python in the standalone directory"
    exit 1
fi

# Run the login manager
"$PYTHON" "$STANDALONE_DIR/../src/login_manager/login_app.py" "$@"
