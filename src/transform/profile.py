# FreeNT Transform Profile
# Copyright (c) 2026, Panoc95
# BSD 3-Clause License

"""
Transform profiles for FreeNT.
This module defines different transformation profiles for converting
Windows into FreeNT with varying levels of component replacement.
"""

from dataclasses import dataclass, field
from typing import Optional, List, Dict, Any
from enum import Enum


class TransformLevel(str, Enum):
    """Transformation levels."""
    MINIMAL = "minimal"      # Only essential changes (identity, shell)
    STANDARD = "standard"    # Replace most non-critical components
    FULL = "full"           # Replace all possible components
    CUSTOM = "custom"       # Custom configuration


@dataclass
class TransformProfile:
    """
    Base class for transformation profiles.
    
    A profile defines which Windows components to replace and
    which Vital-Utilities to install.
    """
    
    name: str
    display_name: str
    description: str
    level: TransformLevel
    
    # Components to replace
    replace_components: List[str] = field(default_factory=list)
    
    # Components to disable
    disable_components: List[str] = field(default_factory=list)
    
    # Components to remove (uninstall)
    remove_components: List[str] = field(default_factory=list)
    
    # Vital-Utilities to install
    install_utilities: List[str] = field(default_factory=list)
    
    # Registry modifications
    registry_modifications: Dict[str, Any] = field(default_factory=dict)
    
    # Environment variable modifications
    environment_modifications: Dict[str, str] = field(default_factory=dict)
    
    # Protected components (should not be touched)
    protected_components: List[str] = field(default_factory=list)
    
    def to_dict(self) -> Dict[str, Any]:
        """Convert to dictionary."""
        return {
            "name": self.name,
            "display_name": self.display_name,
            "description": self.description,
            "level": self.level.value,
            "replace_components": self.replace_components,
            "disable_components": self.disable_components,
            "remove_components": self.remove_components,
            "install_utilities": self.install_utilities,
            "registry_modifications": self.registry_modifications,
            "environment_modifications": self.environment_modifications,
            "protected_components": self.protected_components,
        }
    
    @classmethod
    def from_dict(cls, data: Dict[str, Any]) -> "TransformProfile":
        """Create from dictionary."""
        return cls(
            name=data.get("name", "custom"),
            display_name=data.get("display_name", "Custom Profile"),
            description=data.get("description", ""),
            level=TransformLevel(data.get("level", "custom")),
            replace_components=data.get("replace_components", []),
            disable_components=data.get("disable_components", []),
            remove_components=data.get("remove_components", []),
            install_utilities=data.get("install_utilities", []),
            registry_modifications=data.get("registry_modifications", {}),
            environment_modifications=data.get("environment_modifications", {}),
            protected_components=data.get("protected_components", []),
        )


@dataclass
class MinimalProfile(TransformProfile):
    """
    Minimal transformation profile.
    
    This profile makes the minimal changes necessary to present FreeNT:
    - Changes system identity
    - Replaces Explorer with FreeNT shell
    - Keeps all critical Windows components
    """
    
    def __init__(self):
        super().__init__(
            name="minimal",
            display_name="Minimal",
            description="Minimal transformation - only essential changes",
            level=TransformLevel.MINIMAL,
            replace_components=["explorer"],
            disable_components=[],
            remove_components=[],
            install_utilities=["vital-shell", "vital-filemanager"],
            registry_modifications={
                "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion": {
                    "ProductName": "FreeNT",
                    "DisplayVersion": "1.0",
                    "CurrentBuild": "1.0",
                    "CurrentVersion": "FreeNT 1.0",
                }
            },
            environment_modifications={
                "FREENT": "1",
                "FREENT_VERSION": "1.0",
                "FREENT_PROFILE": "minimal",
            },
            protected_components=[
                "dwm", "csrss", "wininit", "winlogon", "services", 
                "lsass", "svchost"
            ],
        )


@dataclass
class StandardProfile(TransformProfile):
    """
    Standard transformation profile.
    
    This profile replaces most non-critical Windows components with
    Vital-Utilities alternatives:
    - Changes system identity
    - Replaces Explorer with FreeNT shell
    - Replaces common utilities (notepad, calc, paint, etc.)
    - Disables non-essential services
    - Keeps critical Windows components
    """
    
    def __init__(self):
        super().__init__(
            name="standard",
            display_name="Standard",
            description="Standard transformation - replace most non-critical components",
            level=TransformLevel.STANDARD,
            replace_components=[
                "explorer",
                "taskmgr",
                "notepad",
                "calc",
                "paint",
                "wordpad",
                "cmd",
                "powershell",
            ],
            disable_components=[
                "searchindexer",
                "superfetch",
                "windowsupdate",
                "defender",
                "cortana",
            ],
            remove_components=[],
            install_utilities=[
                "vital-shell",
                "vital-taskmgr",
                "vital-notepad",
                "vital-calc",
                "vital-paint",
                "vital-filemanager",
                "vital-browser",
            ],
            registry_modifications={
                "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion": {
                    "ProductName": "FreeNT",
                    "DisplayVersion": "1.0",
                    "CurrentBuild": "1.0",
                    "CurrentVersion": "FreeNT 1.0",
                }
            },
            environment_modifications={
                "FREENT": "1",
                "FREENT_VERSION": "1.0",
                "FREENT_PROFILE": "standard",
            },
            protected_components=[
                "dwm", "csrss", "wininit", "winlogon", "services", 
                "lsass", "svchost"
            ],
        )


@dataclass
class FullProfile(TransformProfile):
    """
    Full transformation profile.
    
    This profile replaces ALL possible Windows components with
    Vital-Utilities alternatives (except DWM and other absolutely required components):
    - Changes system identity
    - Replaces Explorer with FreeNT shell
    - Replaces all utilities
    - Disables all non-essential services
    - Removes unnecessary Windows components
    - Keeps only DWM and absolutely critical components
    """
    
    def __init__(self):
        super().__init__(
            name="full",
            display_name="Full",
            description="Full transformation - replace all possible components",
            level=TransformLevel.FULL,
            replace_components=[
                "explorer",
                "taskmgr",
                "notepad",
                "calc",
                "paint",
                "wordpad",
                "cmd",
                "powershell",
                "edge",
                "iexplore",
                "onenote",
                "skype",
                "xbox",
            ],
            disable_components=[
                "searchindexer",
                "superfetch",
                "windowsupdate",
                "defender",
                "cortana",
                "xbox",
            ],
            remove_components=[
                "edge",
                "iexplore",
                "onenote",
                "skype",
            ],
            install_utilities=[
                "vital-shell",
                "vital-taskmgr",
                "vital-notepad",
                "vital-calc",
                "vital-paint",
                "vital-filemanager",
                "vital-browser",
                "vital-defender",
                "vital-notes",
                "vital-chat",
            ],
            registry_modifications={
                "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion": {
                    "ProductName": "FreeNT",
                    "DisplayVersion": "1.0",
                    "CurrentBuild": "1.0",
                    "CurrentVersion": "FreeNT 1.0",
                    "RegisteredOrganization": "FreeNT Project",
                    "RegisteredOwner": "FreeNT User",
                }
            },
            environment_modifications={
                "FREENT": "1",
                "FREENT_VERSION": "1.0",
                "FREENT_PROFILE": "full",
                "FREENT_FULL_TRANSFORM": "1",
            },
            protected_components=[
                "dwm", "csrss", "wininit", "winlogon", "services", 
                "lsass", "svchost"
            ],
        )


@dataclass
class CustomProfile(TransformProfile):
    """
    Custom transformation profile.
    
    This profile allows custom configuration of which components
    to replace and which utilities to install.
    """
    
    def __init__(
        self,
        name: str = "custom",
        display_name: str = "Custom",
        description: str = "Custom transformation profile",
        replace_components: Optional[List[str]] = None,
        disable_components: Optional[List[str]] = None,
        remove_components: Optional[List[str]] = None,
        install_utilities: Optional[List[str]] = None,
        registry_modifications: Optional[Dict[str, Any]] = None,
        environment_modifications: Optional[Dict[str, str]] = None,
        protected_components: Optional[List[str]] = None,
    ):
        super().__init__(
            name=name,
            display_name=display_name,
            description=description,
            level=TransformLevel.CUSTOM,
            replace_components=replace_components or [],
            disable_components=disable_components or [],
            remove_components=remove_components or [],
            install_utilities=install_utilities or [],
            registry_modifications=registry_modifications or {},
            environment_modifications=environment_modifications or {},
            protected_components=protected_components or [
                "dwm", "csrss", "wininit", "winlogon", "services", 
                "lsass", "svchost"
            ],
        )


# Profile registry
PROFILES: Dict[str, TransformProfile] = {
    "minimal": MinimalProfile(),
    "standard": StandardProfile(),
    "full": FullProfile(),
}


def get_profile(name: str) -> TransformProfile:
    """
    Get a transformation profile by name.
    
    Args:
        name: Name of the profile.
        
    Returns:
        TransformProfile object.
        
    Raises:
        KeyError: If profile not found.
    """
    if name not in PROFILES:
        # Try to create a custom profile
        return CustomProfile(name=name)
    return PROFILES[name]


def list_profiles() -> List[TransformProfile]:
    """
    List all available transformation profiles.
    
    Returns:
        List of TransformProfile objects.
    """
    return list(PROFILES.values())


def create_profile(
    name: str,
    display_name: str,
    description: str,
    level: str = "custom",
    **kwargs,
) -> TransformProfile:
    """
    Create a custom transformation profile.
    
    Args:
        name: Profile name.
        display_name: Display name.
        description: Description.
        level: Transformation level (minimal, standard, full, custom).
        **kwargs: Additional profile parameters.
        
    Returns:
        TransformProfile object.
    """
    return CustomProfile(
        name=name,
        display_name=display_name,
        description=description,
        **kwargs,
    )


if __name__ == "__main__":
    # Example usage
    print("FreeNT Transformation Profiles")
    print("==============================")
    
    profiles = list_profiles()
    for profile in profiles:
        print(f"\n{profile.display_name} ({profile.name})")
        print(f"  Level: {profile.level.value}")
        print(f"  Description: {profile.description}")
        print(f"  Replace: {profile.replace_components}")
        print(f"  Install: {profile.install_utilities}")
