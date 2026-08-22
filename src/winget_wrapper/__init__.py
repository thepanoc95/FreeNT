# FreeNT Winget Wrapper
# Copyright (c) 2026, Panoc95
# BSD 3-Clause License

"""
Winget wrapper for FreeNT.
Provides a convenient interface to Microsoft's winget package manager.
"""

from .winget import WingetWrapper, WingetError, PackageInfo

__all__ = ["WingetWrapper", "WingetError", "PackageInfo"]
