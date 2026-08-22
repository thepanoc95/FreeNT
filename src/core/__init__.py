# FreeNT Core
# Copyright (c) 2026, Panoc95
# BSD 3-Clause License

"""
Core functionality for FreeNT.
Provides essential utilities and configuration management.
"""

from .config import FreeNTConfig, ConfigError
from .utils import (
    detect_windows_version,
    check_admin_privileges,
    run_command,
    run_command_async,
)

__all__ = [
    "FreeNTConfig",
    "ConfigError",
    "detect_windows_version",
    "check_admin_privileges",
    "run_command",
    "run_command_async",
]
