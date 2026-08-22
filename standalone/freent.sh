#!/bin/sh
# FreeNT Standalone Launcher (Bash/Shell compatible)
# Copyright (c) 2026, Panoc95
# BSD 3-Clause License

# This script launches FreeNT in standalone mode
# It works with bash, sh, dash, and other POSIX-compatible shells

# Set standalone directory
STANDALONE_DIR=$(dirname "$0")

# Function to check if command exists
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Function to find python in various locations
find_python() {
    # Check system python
    if command_exists python; then
        echo python
        return 0
    fi
    
    # Check python3
    if command_exists python3; then
        echo python3
        return 0
    fi
    
    # Check portable python in standalone directory
    if [ -f "$STANDALONE_DIR/python/python" ]; then
        echo "$STANDALONE_DIR/python/python"
        return 0
    fi
    
    # Check portable python in parent directory
    if [ -f "$STANDALONE_DIR/../python/python" ]; then
        echo "$STANDALONE_DIR/../python/python"
        return 0
    fi
    
    # Check in PATH
    if command_exists python; then
        echo python
        return 0
    fi
    
    return 1
}

# Find python
PYTHON=$(find_python)

if [ -z "$PYTHON" ]; then
    echo "Error: Python is required to run FreeNT"
    echo "Please install Python or place a portable Python in the standalone directory"
    echo "Download Python from: https://www.python.org/downloads/"
    exit 1
fi

# Run the login manager
"$PYTHON" "$STANDALONE_DIR/../src/login_manager/login_app.py" "$@"
