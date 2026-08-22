#!/bin/sh
# FreeNT CLI Launcher (Bash/Shell compatible)
# Copyright (c) 2026, Panoc95
# BSD 3-Clause License

# Set standalone directory
STANDALONE_DIR=$(dirname "$0")

# Function to check if command exists
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Function to find python
find_python() {
    if command_exists python; then
        echo python
        return 0
    fi
    
    if command_exists python3; then
        echo python3
        return 0
    fi
    
    if [ -f "$STANDALONE_DIR/python/python" ]; then
        echo "$STANDALONE_DIR/python/python"
        return 0
    fi
    
    if [ -f "$STANDALONE_DIR/../python/python" ]; then
        echo "$STANDALONE_DIR/../python/python"
        return 0
    fi
    
    if command_exists python; then
        echo python
        return 0
    fi
    
    return 1
}

# Find python
PYTHON=$(find_python)

if [ -z "$PYTHON" ]; then
    echo "Error: Python is required to run FreeNT CLI"
    exit 1
fi

# Run CLI
"$PYTHON" "$STANDALONE_DIR/../src/cli.py" "$@"
