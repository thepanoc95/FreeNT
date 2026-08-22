# FreeNT Core - Utilities
# Copyright (c) 2026, Panoc95
# BSD 3-Clause License

"""
Utility functions for FreeNT.
This module provides various utility functions for system operations.
"""

import asyncio
import os
import platform
import subprocess
import sys
from typing import Optional, List, Tuple, Union


def detect_windows_version() -> Tuple[int, int, int]:
    """
    Detect the Windows version.
    
    Returns:
        Tuple of (major, minor, build) version numbers.
        For example, Windows 11 22H2 returns (10, 0, 22621).
    """
    if sys.platform != "win32":
        raise RuntimeError("This function is only available on Windows.")
    
    try:
        version = sys.getwindowsversion()
        return (version.major, version.minor, version.build)
    except AttributeError:
        # Fallback for older Python versions
        import ctypes
        from ctypes import wintypes
        
        class OSVERSIONINFOEXW(ctypes.Structure):
            _fields_ = [
                ("dwOSVersionInfoSize", wintypes.DWORD),
                ("dwMajorVersion", wintypes.DWORD),
                ("dwMinorVersion", wintypes.DWORD),
                ("dwBuildNumber", wintypes.DWORD),
                ("dwPlatformId", wintypes.DWORD),
                ("szCSDVersion", wintypes.WCHAR * 128),
                ("wServicePackMajor", wintypes.WORD),
                ("wServicePackMinor", wintypes.WORD),
                ("wSuiteMask", wintypes.WORD),
                ("wProductType", wintypes.BYTE),
                ("wReserved", wintypes.BYTE),
            ]
        
        GetVersionEx = ctypes.windll.kernel32.GetVersionExW
        GetVersionEx.argtypes = [ctypes.POINTER(OSVERSIONINFOEXW)]
        
        osvi = OSVERSIONINFOEXW()
        osvi.dwOSVersionInfoSize = ctypes.sizeof(OSVERSIONINFOEXW)
        
        if GetVersionEx(ctypes.byref(osvi)):
            return (osvi.dwMajorVersion, osvi.dwMinorVersion, osvi.dwBuildNumber)
        else:
            raise RuntimeError("Failed to get Windows version.")


def check_admin_privileges() -> bool:
    """
    Check if the current process has administrator privileges.
    
    Returns:
        True if running with admin privileges, False otherwise.
    """
    if sys.platform != "win32":
        # On non-Windows, assume admin if running as root
        return os.geteuid() == 0
    
    try:
        import ctypes
        from ctypes import wintypes
        
        # Method 1: Check token information
        try:
            TokenElevation = 20
            TokenInformationClass = ctypes.c_int
            
            GetCurrentProcess = ctypes.windll.kernel32.GetCurrentProcess
            OpenProcessToken = ctypes.windll.advapi32.OpenProcessToken
            GetTokenInformation = ctypes.windll.advapi32.GetTokenInformation
            
            TOKEN_QUERY = 0x0008
            
            hToken = wintypes.HANDLE()
            if not OpenProcessToken(
                GetCurrentProcess(),
                TOKEN_QUERY,
                ctypes.byref(hToken)
            ):
                return False
            
            result = wintypes.DWORD()
            return_length = wintypes.DWORD()
            
            if GetTokenInformation(
                hToken,
                TokenInformationClass(TokenElevation),
                ctypes.byref(result),
                ctypes.sizeof(result),
                ctypes.byref(return_length)
            ):
                return bool(result.value)
        except Exception:
            pass
        
        # Method 2: Try to write to a protected registry key
        try:
            import winreg
            key = winreg.OpenKey(
                winreg.HKEY_LOCAL_MACHINE,
                r"SOFTWARE\Microsoft\Windows\CurrentVersion\Run",
                0,
                winreg.KEY_WRITE
            )
            winreg.CloseKey(key)
            return True
        except Exception:
            return False
            
    except ImportError:
        # Fallback: check if we can write to Program Files
        try:
            test_path = os.path.join(os.environ.get("ProgramFiles", "C:\\Program Files"), "test.txt")
            with open(test_path, "w") as f:
                f.write("test")
            os.remove(test_path)
            return True
        except Exception:
            return False


def run_command(
    command: Union[str, List[str]],
    cwd: Optional[str] = None,
    env: Optional[dict] = None,
    capture_output: bool = False,
    timeout: Optional[float] = None,
    shell: bool = False,
) -> subprocess.CompletedProcess[str]:
    """
    Run a command synchronously.
    
    Args:
        command: Command to run as string or list of strings.
        cwd: Working directory for the command.
        env: Environment variables for the command.
        capture_output: Whether to capture stdout and stderr.
        timeout: Timeout in seconds.
        shell: Whether to use shell.
        
    Returns:
        CompletedProcess with command results.
    """
    if isinstance(command, str):
        if shell:
            cmd = command
        else:
            cmd = command.split()
    else:
        cmd = command
    
    try:
        result = subprocess.run(
            cmd,
            cwd=cwd,
            env=env,
            capture_output=capture_output,
            text=True,
            timeout=timeout,
            shell=shell,
            check=False,
        )
        return result
    except subprocess.TimeoutExpired as e:
        raise TimeoutError(f"Command timed out after {timeout} seconds: {command}") from e


async def run_command_async(
    command: Union[str, List[str]],
    cwd: Optional[str] = None,
    env: Optional[dict] = None,
    capture_output: bool = False,
    timeout: Optional[float] = None,
    shell: bool = False,
) -> subprocess.CompletedProcess[str]:
    """
    Run a command asynchronously.
    
    Args:
        command: Command to run as string or list of strings.
        cwd: Working directory for the command.
        env: Environment variables for the command.
        capture_output: Whether to capture stdout and stderr.
        timeout: Timeout in seconds.
        shell: Whether to use shell.
        
    Returns:
        CompletedProcess with command results.
    """
    if isinstance(command, str):
        if shell:
            cmd = command
        else:
            cmd = command.split()
    else:
        cmd = command
    
    try:
        proc = await asyncio.create_subprocess_exec(
            *cmd,
            cwd=cwd,
            env=env,
            stdout=asyncio.subprocess.PIPE if capture_output else None,
            stderr=asyncio.subprocess.PIPE if capture_output else None,
            shell=shell,
        )
        
        stdout, stderr = await asyncio.wait_for(
            proc.communicate(),
            timeout=timeout,
        )
        
        return subprocess.CompletedProcess(
            args=cmd,
            returncode=proc.returncode,
            stdout=stdout.decode() if stdout else "",
            stderr=stderr.decode() if stderr else "",
        )
    except asyncio.TimeoutError as e:
        proc.kill()
        raise TimeoutError(f"Command timed out after {timeout} seconds: {command}") from e


def get_system_info() -> dict:
    """
    Get system information.
    
    Returns:
        Dictionary with system information.
    """
    info = {
        "platform": sys.platform,
        "python_version": sys.version,
        "python_executable": sys.executable,
    }
    
    if sys.platform == "win32":
        try:
            version = sys.getwindowsversion()
            info["windows_version"] = {
                "major": version.major,
                "minor": version.minor,
                "build": version.build,
                "platform": version.platform,
                "service_pack": version.service_pack,
            }
        except AttributeError:
            info["windows_version"] = detect_windows_version()
        
        info["is_admin"] = check_admin_privileges()
    
    return info


def get_environment_info() -> dict:
    """
    Get environment information.
    
    Returns:
        Dictionary with environment information.
    """
    info = {
        "path": os.getenv("PATH", ""),
        "home": os.getenv("HOME", os.getenv("USERPROFILE", "")),
        "temp": os.getenv("TEMP", os.getenv("TMP", "")),
    }
    
    # Check for common tools
    tools = [
        "gcc", "g++", "make", "gnumake",
        "clang", "clang++", "bsdmake",
        "python", "python3", "pip", "pip3",
        "winget", "powershell", "cmd",
    ]
    
    info["available_tools"] = {}
    for tool in tools:
        info["available_tools"][tool] = which(tool) is not None
    
    return info


def which(executable: str) -> Optional[str]:
    """
    Find the full path of an executable.
    
    Args:
        executable: Name of the executable to find.
        
    Returns:
        Full path to the executable, or None if not found.
    """
    try:
        import shutil
        return shutil.which(executable)
    except ImportError:
        # Fallback implementation
        paths = os.getenv("PATH", "").split(os.pathsep)
        for path in paths:
            full_path = os.path.join(path, executable)
            if os.path.exists(full_path) and os.access(full_path, os.X_OK):
                return full_path
        return None


def format_bytes(size: int) -> str:
    """
    Format bytes to human-readable string.
    
    Args:
        size: Size in bytes.
        
    Returns:
        Human-readable size string.
    """
    for unit in ["B", "KB", "MB", "GB", "TB"]:
        if size < 1024.0:
            return f"{size:.2f} {unit}"
        size /= 1024.0
    return f"{size:.2f} PB"


def parse_version(version: str) -> Tuple[int, ...]:
    """
    Parse a version string into tuple of integers.
    
    Args:
        version: Version string (e.g., "1.2.3" or "1.2.3.4").
        
    Returns:
        Tuple of integers representing the version.
    """
    parts = version.split(".")
    return tuple(int(part) for part in parts if part.isdigit())


def compare_versions(v1: str, v2: str) -> int:
    """
    Compare two version strings.
    
    Args:
        v1: First version string.
        v2: Second version string.
        
    Returns:
        -1 if v1 < v2, 0 if v1 == v2, 1 if v1 > v2.
    """
    v1_parts = parse_version(v1)
    v2_parts = parse_version(v2)
    
    # Pad with zeros to make equal length
    max_len = max(len(v1_parts), len(v2_parts))
    v1_parts = v1_parts + (0,) * (max_len - len(v1_parts))
    v2_parts = v2_parts + (0,) * (max_len - len(v2_parts))
    
    for a, b in zip(v1_parts, v2_parts):
        if a < b:
            return -1
        elif a > b:
            return 1
    return 0
