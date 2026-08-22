#!/bin/sh
# FreeNT CLI Minimal Shell Launcher
# Copyright (c) 2026, Panoc95
# BSD 3-Clause License

# Minimal version for basic shells and BusyBox

# Set standalone directory
STANDALONE_DIR="$0"
while [ -L "$STANDALONE_DIR" ]; do
    STANDALONE_DIR=$(readlink "$STANDALONE_DIR" 2>/dev/null || echo "")
    if [ -z "$STANDALONE_DIR" ]; then break; fi
    case "$STANDALONE_DIR" in
        /*) ;;
        *) STANDALONE_DIR="$(dirname "$STANDALONE_DIR")/$STANDALONE_DIR" ;;
    esac
done
STANDALONE_DIR=$(dirname "$STANDALONE_DIR")

# Simple command check
check_cmd() {
    if command -v "$1" >/dev/null 2>&1; then return 0; fi
    if which "$1" >/dev/null 2>&1; then return 0; fi
    OLD_IFS="$IFS"; IFS=":"
    for p in $PATH; do
        if [ -n "$p" ] && [ -x "$p/$1" ]; then
            IFS="$OLD_IFS"
            return 0
        fi
    done
    IFS="$OLD_IFS"
    return 1
}

# Find python
find_python() {
    if check_cmd python; then echo python; return 0; fi
    if check_cmd python3; then echo python3; return 0; fi
    if [ -x "$STANDALONE_DIR/python" ]; then echo "$STANDALONE_DIR/python"; return 0; fi
    if [ -x "$STANDALONE_DIR/python/python" ]; then echo "$STANDALONE_DIR/python/python"; return 0; fi
    if [ -x "$STANDALONE_DIR/../python" ]; then echo "$STANDALONE_DIR/../python"; return 0; fi
    if [ -x "$STANDALONE_DIR/../python/python" ]; then echo "$STANDALONE_DIR/../python/python"; return 0; fi
    return 1
}

PYTHON=$(find_python)
if [ -z "$PYTHON" ]; then
    echo "Error: Python is required to run FreeNT CLI"
    exit 1
fi

# Set PYTHONPATH for portable Python
if echo "$PYTHON" | grep -q "$STANDALONE_DIR"; then
    if [ -d "$STANDALONE_DIR/../src" ]; then
        export PYTHONPATH="$STANDALONE_DIR/../src:$STANDALONE_DIR/../standalone/lib:$PYTHONPATH"
    fi
fi

# Run CLI
"$PYTHON" "$STANDALONE_DIR/../src/cli.py" "$@"
