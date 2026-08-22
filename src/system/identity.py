# FreeNT System Identity Management
# Copyright (c) 2026, Panoc95
# BSD 3-Clause License

"""
System identity modification for FreeNT.
This module changes the system identity to present FreeNT instead of Windows.
"""

import ctypes
import os
import sys
import platform
from dataclasses import dataclass
from typing import Optional, Dict, Any
from ctypes import wintypes


@dataclass
class SystemIdentity:
    """Represents the current system identity."""
    
    # Original system information
    original_platform: str
    original_system: str
    original_release: str
    original_version: str
    original_machine: str
    original_node: str
    
    # Modified system information
    modified_platform: str = "FreeNT"
    modified_system: str = "FreeNT"
    modified_release: str = "1.0"
    modified_version: str = "FreeNT 1.0"
    modified_machine: str = "FreeNT"
    modified_node: str = "FreeNT"
    
    # Branding
    product_name: str = "FreeNT"
    product_version: str = "1.0"
    manufacturer: str = "FreeNT Project"
    description: str = "FreeNT - Alternative Userland for Windows NT"
    
    def to_dict(self) -> Dict[str, Any]:
        """Convert to dictionary."""
        return {
            "original": {
                "platform": self.original_platform,
                "system": self.original_system,
                "release": self.original_release,
                "version": self.original_version,
                "machine": self.original_machine,
                "node": self.original_node,
            },
            "modified": {
                "platform": self.modified_platform,
                "system": self.modified_system,
                "release": self.modified_release,
                "version": self.modified_version,
                "machine": self.modified_machine,
                "node": self.modified_node,
            },
            "branding": {
                "product_name": self.product_name,
                "product_version": self.product_version,
                "manufacturer": self.manufacturer,
                "description": self.description,
            }
        }


class SystemIdentityError(Exception):
    """Base exception for system identity errors."""
    pass


class RegistryModificationError(SystemIdentityError):
    """Raised when registry modification fails."""
    pass


class BrandingModificationError(SystemIdentityError):
    """Raised when branding modification fails."""
    pass


def get_current_identity() -> SystemIdentity:
    """
    Get the current system identity.
    
    Returns:
        SystemIdentity object with current system information.
    """
    return SystemIdentity(
        original_platform=sys.platform,
        original_system=platform.system(),
        original_release=platform.release(),
        original_version=platform.version(),
        original_machine=platform.machine(),
        original_node=platform.node(),
    )


def modify_system_identity(
    identity: Optional[SystemIdentity] = None,
    apply: bool = True,
) -> SystemIdentity:
    """
    Modify the system identity to present FreeNT.
    
    This function changes various system properties to make the system
    appear as FreeNT instead of Windows. Note that some changes require
    administrator privileges and may not persist across reboots.
    
    Args:
        identity: Optional SystemIdentity to use. If None, creates new.
        apply: Whether to apply the changes immediately.
        
    Returns:
        SystemIdentity object with the modified identity.
    """
    if identity is None:
        identity = get_current_identity()
    
    if apply:
        _apply_system_identity(identity)
    
    return identity


def _apply_system_identity(identity: SystemIdentity) -> None:
    """
    Apply system identity changes.
    
    Args:
        identity: SystemIdentity to apply.
    """
    try:
        # Modify platform module (affects Python's sys.platform)
        sys.platform = identity.modified_platform
        
        # Modify platform module attributes
        platform.system = lambda: identity.modified_system
        platform.release = lambda: identity.modified_release
        platform.version = lambda: identity.modified_version
        platform.machine = lambda: identity.modified_machine
        platform.node = lambda: identity.modified_node
        
        # Modify os module
        os.name = "freent"
        
        # Modify environment variables
        os.environ["FREENT_IDENTITY"] = "1"
        os.environ["FREENT_PLATFORM"] = identity.modified_platform
        os.environ["FREENT_SYSTEM"] = identity.modified_system
        
        # Windows-specific modifications
        if sys.platform.startswith("win"):
            _modify_windows_identity(identity)
        
    except Exception as e:
        raise SystemIdentityError(f"Failed to apply system identity: {e}") from e


def _modify_windows_identity(identity: SystemIdentity) -> None:
    """
    Apply Windows-specific identity modifications.
    
    Args:
        identity: SystemIdentity to apply.
    """
    try:
        # Modify registry to change system branding
        _modify_registry_branding(identity)
        
        # Modify WMI information
        _modify_wmi_information(identity)
        
        # Modify environment variables
        _modify_environment_variables(identity)
        
    except Exception as e:
        raise BrandingModificationError(f"Failed to modify Windows identity: {e}") from e


def _modify_registry_branding(identity: SystemIdentity) -> None:
    """
    Modify Windows registry to change system branding.
    
    This requires administrator privileges.
    
    Args:
        identity: SystemIdentity to apply.
    """
    try:
        import winreg
        
        # Modify CurrentVersion registry keys
        key_paths = [
            r"SOFTWARE\Microsoft\Windows NT\CurrentVersion",
            r"SOFTWARE\Microsoft\Windows\CurrentVersion",
        ]
        
        for key_path in key_paths:
            try:
                with winreg.OpenKey(
                    winreg.HKEY_LOCAL_MACHINE,
                    key_path,
                    0,
                    winreg.KEY_WRITE
                ) as key:
                    # Modify product name
                    winreg.SetValueEx(key, "ProductName", 0, winreg.REG_SZ, identity.product_name)
                    
                    # Modify display version
                    winreg.SetValueEx(key, "DisplayVersion", 0, winreg.REG_SZ, identity.product_version)
                    
                    # Modify current build
                    winreg.SetValueEx(key, "CurrentBuild", 0, winreg.REG_SZ, identity.product_version)
                    
                    # Modify current version
                    winreg.SetValueEx(key, "CurrentVersion", 0, winreg.REG_SZ, identity.modified_version)
                    
            except Exception:
                # Key might not exist or we might not have access
                pass
        
        # Modify registered organization
        try:
            with winreg.OpenKey(
                winreg.HKEY_LOCAL_MACHINE,
                r"SOFTWARE\Microsoft\Windows NT\CurrentVersion",
                0,
                winreg.KEY_WRITE
            ) as key:
                winreg.SetValueEx(key, "RegisteredOrganization", 0, winreg.REG_SZ, identity.manufacturer)
                winreg.SetValueEx(key, "RegisteredOwner", 0, winreg.REG_SZ, identity.manufacturer)
        except Exception:
            pass
        
        # Broadcast WM_SETTINGCHANGE to update system
        _broadcast_setting_change()
        
    except ImportError:
        # winreg not available (not Windows)
        pass
    except Exception as e:
        raise RegistryModificationError(f"Failed to modify registry: {e}") from e


def _modify_wmi_information(identity: SystemIdentity) -> None:
    """
    Modify WMI information to reflect FreeNT.
    
    Args:
        identity: SystemIdentity to apply.
    """
    try:
        import wmi
        
        c = wmi.WMI()
        
        # Modify Win32_OperatingSystem
        for os_obj in c.Win32_OperatingSystem():
            try:
                os_obj.Caption = identity.product_name
                os_obj.Version = identity.product_version
                os_obj.put_()
            except Exception:
                pass
        
    except ImportError:
        # wmi not available
        pass
    except Exception:
        # WMI modification might fail
        pass


def _modify_environment_variables(identity: SystemIdentity) -> None:
    """
    Modify environment variables to reflect FreeNT.
    
    Args:
        identity: SystemIdentity to apply.
    """
    # Set environment variables
    os.environ["FREENT"] = "1"
    os.environ["FREENT_VERSION"] = identity.product_version
    os.environ["FREENT_SYSTEM"] = identity.product_name
    
    # Override Windows-specific variables
    os.environ["OS"] = identity.product_name
    os.environ["WINDIR"] = os.environ.get("FREENT_DIR", os.path.join(os.environ.get("ProgramFiles", "C:\\Program Files"), "FreeNT"))


def _broadcast_setting_change() -> None:
    """
    Broadcast WM_SETTINGCHANGE to notify system of changes.
    """
    try:
        HWND_BROADCAST = 0xFFFF
        WM_SETTINGCHANGE = 0x001A
        
        user32 = ctypes.windll.user32
        user32.SendMessageTimeoutW(
            HWND_BROADCAST,
            WM_SETTINGCHANGE,
            0,
            "Environment",
            0,
            5000,
            None
        )
    except Exception:
        pass


def restore_system_identity() -> None:
    """
    Restore the original system identity.
    
    This function attempts to restore the original system identity.
    Note that some changes may require a reboot to fully restore.
    """
    try:
        # Reload platform module
        import importlib
        importlib.reload(platform)
        importlib.reload(sys)
        
        # Remove FreeNT environment variables
        for key in list(os.environ.keys()):
            if key.startswith("FREENT_"):
                del os.environ[key]
        
        # Restore Windows-specific settings
        if sys.platform.startswith("win"):
            _restore_windows_identity()
        
    except Exception as e:
        raise SystemIdentityError(f"Failed to restore system identity: {e}") from e


def _restore_windows_identity() -> None:
    """
    Restore Windows-specific identity.
    """
    try:
        import winreg
        
        # This is a simplified restore - in practice, we'd need to know the original values
        # For now, we just remove FreeNT-specific registry entries
        try:
            with winreg.OpenKey(
                winreg.HKEY_LOCAL_MACHINE,
                r"SOFTWARE\FreeNT",
                0,
                winreg.KEY_WRITE
            ) as key:
                winreg.DeleteKey(key, "Identity")
        except Exception:
            pass
        
        # Broadcast setting change
        _broadcast_setting_change()
        
    except ImportError:
        pass
    except Exception:
        pass


def get_system_branding() -> Dict[str, str]:
    """
    Get current system branding information.
    
    Returns:
        Dictionary with system branding information.
    """
    branding = {
        "platform": sys.platform,
        "system": platform.system(),
        "release": platform.release(),
        "version": platform.version(),
        "machine": platform.machine(),
        "node": platform.node(),
    }
    
    # Check if FreeNT identity is applied
    if os.environ.get("FREENT_IDENTITY") == "1":
        branding["freent"] = True
        branding["freent_version"] = os.environ.get("FREENT_VERSION", "1.0")
    else:
        branding["freent"] = False
    
    return branding


if __name__ == "__main__":
    # Example usage
    print("Current system identity:")
    identity = get_current_identity()
    print(f"  Platform: {identity.original_platform}")
    print(f"  System: {identity.original_system}")
    print(f"  Release: {identity.original_release}")
    print(f"  Version: {identity.original_version}")
    
    print("\nApplying FreeNT identity...")
    modified = modify_system_identity(identity)
    
    print("\nModified system identity:")
    print(f"  Platform: {sys.platform}")
    print(f"  System: {platform.system()}")
    print(f"  Release: {platform.release()}")
    print(f"  Version: {platform.version()}")
    
    print("\nBranding:")
    branding = get_system_branding()
    for key, value in branding.items():
        print(f"  {key}: {value}")
