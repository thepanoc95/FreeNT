#!/bin/sh
# FreeNT Startup Script (Bash/Shell compatible)
# Copyright (c) 2026, Panoc95
# BSD 3-Clause License

# This script sets up environment variables and launches FreeNT

# Set standalone directory
STANDALONE_DIR=$(dirname "$0")

# Export FreeNT environment variables
export FREENT_HOME="$STANDALONE_DIR/.."

# Add standalone bin to PATH if not already there
case ":$PATH:" in
    *":$STANDALONE_DIR/bin:"*) ;;
    *) export PATH="$PATH:$STANDALONE_DIR/bin" ;;
esac

# Check for portable Python and add to PATH
if [ -d "$STANDALONE_DIR/python" ]; then
    case ":$PATH:" in
        *":$STANDALONE_DIR/python:"*) ;;
        *) export PATH="$PATH:$STANDALONE_DIR/python" ;;
    esac
fi

# Launch the login manager
"$STANDALONE_DIR/freent.sh" "$@"
