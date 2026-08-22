# FreeNT Vital-Utilities Integration
# Copyright (c) 2026, Panoc95
# BSD 3-Clause License

"""
Vital-Utilities integration for FreeNT.
This module handles downloading, installing, and managing Vital-Utilities
as a replacement for Windows components.
"""

import os
import sys
import json
import shutil
import subprocess
import tempfile
import urllib.request
from pathlib import Path
from typing import Optional, List, Dict, Any, Union
from dataclasses import dataclass, field


@dataclass
class VitalUtility:
    """Represents a Vital-Utility component."""
    
    name: str
    display_name: str
    description: str
    repository: str
    version: str = "latest"
    download_url: Optional[str] = None
    executable: Optional[str] = None
    dependencies: List[str] = field(default_factory=list)
    replaces: List[str] = field(default_factory=list)  # Windows components it replaces
    category: str = "utility"
    installed: bool = False
    
    def to_dict(self) -> Dict[str, Any]:
        """Convert to dictionary."""
        return {
            "name": self.name,
            "display_name": self.display_name,
            "description": self.description,
            "repository": self.repository,
            "version": self.version,
            "download_url": self.download_url,
            "executable": self.executable,
            "dependencies": self.dependencies,
            "replaces": self.replaces,
            "category": self.category,
            "installed": self.installed,
        }


class VitalUtilitiesError(Exception):
    """Base exception for Vital-Utilities errors."""
    pass


class DownloadError(VitalUtilitiesError):
    """Raised when download fails."""
    pass


class InstallationError(VitalUtilitiesError):
    """Raised when installation fails."""
    pass


class VitalUtilitiesManager:
    """
    Manages Vital-Utilities components.
    
    This class handles downloading, installing, and updating Vital-Utilities
    as replacements for Windows components.
    """
    
    # Vital-Utilities repository
    VITAL_REPO = "https://github.com/Vital-Utilities/Vital-Utilities"
    VITAL_API = "https://api.github.com/repos/Vital-Utilities/Vital-Utilities"
    
    # Default installation directory
    DEFAULT_INSTALL_DIR = os.path.join(
        os.environ.get("ProgramFiles", "C:\\Program Files"),
        "Vital-Utilities"
    )
    
    # List of known Vital-Utilities
    KNOWN_UTILITIES: Dict[str, VitalUtility] = {
        "vital-shell": VitalUtility(
            name="vital-shell",
            display_name="Vital Shell",
            description="Replacement for cmd.exe and PowerShell",
            repository="Vital-Utilities/Vital-Utilities",
            download_url=None,  # Will be determined
            executable="vital-shell.exe",
            replaces=["cmd", "powershell"],
            category="shell",
        ),
        "vital-taskmgr": VitalUtility(
            name="vital-taskmgr",
            display_name="Vital Task Manager",
            description="Replacement for Windows Task Manager",
            repository="Vital-Utilities/Vital-Utilities",
            executable="vital-taskmgr.exe",
            replaces=["taskmgr"],
            category="system",
        ),
        "vital-notepad": VitalUtility(
            name="vital-notepad",
            display_name="Vital Notepad",
            description="Replacement for Notepad",
            repository="Vital-Utilities/Vital-Utilities",
            executable="vital-notepad.exe",
            replaces=["notepad"],
            category="utility",
        ),
        "vital-calc": VitalUtility(
            name="vital-calc",
            display_name="Vital Calculator",
            description="Replacement for Windows Calculator",
            repository="Vital-Utilities/Vital-Utilities",
            executable="vital-calc.exe",
            replaces=["calc"],
            category="utility",
        ),
        "vital-paint": VitalUtility(
            name="vital-paint",
            display_name="Vital Paint",
            description="Replacement for MS Paint",
            repository="Vital-Utilities/Vital-Utilities",
            executable="vital-paint.exe",
            replaces=["paint"],
            category="utility",
        ),
        "vital-browser": VitalUtility(
            name="vital-browser",
            display_name="Vital Browser",
            description="Lightweight web browser",
            repository="Vital-Utilities/Vital-Utilities",
            executable="vital-browser.exe",
            replaces=["edge", "iexplore"],
            category="browser",
        ),
        "vital-defender": VitalUtility(
            name="vital-defender",
            display_name="Vital Defender",
            description="Security and antivirus",
            repository="Vital-Utilities/Vital-Utilities",
            executable="vital-defender.exe",
            replaces=["defender"],
            category="security",
        ),
        "vital-notes": VitalUtility(
            name="vital-notes",
            display_name="Vital Notes",
            description="Note-taking application",
            repository="Vital-Utilities/Vital-Utilities",
            executable="vital-notes.exe",
            replaces=["onenote"],
            category="utility",
        ),
        "vital-chat": VitalUtility(
            name="vital-chat",
            display_name="Vital Chat",
            description="Communication application",
            repository="Vital-Utilities/Vital-Utilities",
            executable="vital-chat.exe",
            replaces=["skype"],
            category="communication",
        ),
        "vital-filemanager": VitalUtility(
            name="vital-filemanager",
            display_name="Vital File Manager",
            description="Replacement for File Explorer",
            repository="Vital-Utilities/Vital-Utilities",
            executable="vital-filemanager.exe",
            replaces=["explorer"],
            category="filemanager",
        ),
    }
    
    def __init__(self, install_dir: Optional[str] = None):
        """
        Initialize the Vital-Utilities manager.
        
        Args:
            install_dir: Installation directory for Vital-Utilities.
        """
        self.install_dir = install_dir or self.DEFAULT_INSTALL_DIR
        self.installed_utilities: Dict[str, VitalUtility] = {}
        self._load_installed_utilities()
    
    def _load_installed_utilities(self) -> None:
        """Load list of installed utilities."""
        self.installed_utilities.clear()
        
        # Check for installed utilities
        for name, utility in self.KNOWN_UTILITIES.items():
            # Check if executable exists
            exec_path = os.path.join(self.install_dir, utility.executable or name)
            if os.path.exists(exec_path):
                utility.installed = True
                self.installed_utilities[name] = utility
            else:
                utility.installed = False
    
    def list_utilities(self, category: Optional[str] = None) -> List[VitalUtility]:
        """
        List available Vital-Utilities.
        
        Args:
            category: Optional category filter.
            
        Returns:
            List of VitalUtility objects.
        """
        utilities = list(self.KNOWN_UTILITIES.values())
        
        if category:
            utilities = [u for u in utilities if u.category == category]
        
        return utilities
    
    def get_utility(self, name: str) -> VitalUtility:
        """
        Get a Vital-Utility by name.
        
        Args:
            name: Name of the utility.
            
        Returns:
            VitalUtility object.
            
        Raises:
            KeyError: If utility not found.
        """
        if name not in self.KNOWN_UTILITIES:
            raise KeyError(f"Utility '{name}' not found")
        return self.KNOWN_UTILITIES[name]
    
    def is_installed(self, name: str) -> bool:
        """
        Check if a Vital-Utility is installed.
        
        Args:
            name: Name of the utility.
            
        Returns:
            True if installed, False otherwise.
        """
        return name in self.installed_utilities
    
    def ensure_vital_utilities(
        self,
        utilities: Optional[List[str]] = None,
        all_utilities: bool = False,
        force_reinstall: bool = False,
    ) -> Dict[str, bool]:
        """
        Ensure Vital-Utilities are installed.
        
        Args:
            utilities: List of utility names to install. If None and all_utilities is False,
                      installs utilities that replace removed Windows components.
            all_utilities: If True, install all known utilities.
            force_reinstall: If True, reinstall even if already installed.
            
        Returns:
            Dictionary of utility names and installation success status.
        """
        results = {}
        
        if all_utilities:
            utilities = list(self.KNOWN_UTILITIES.keys())
        elif utilities is None:
            # Install utilities that replace common Windows components
            utilities = [
                "vital-shell",
                "vital-taskmgr",
                "vital-notepad",
                "vital-calc",
                "vital-paint",
                "vital-browser",
                "vital-filemanager",
            ]
        
        for name in utilities:
            try:
                if self.is_installed(name) and not force_reinstall:
                    results[name] = True
                    continue
                
                utility = self.get_utility(name)
                success = self.install_utility(utility)
                results[name] = success
                
                if success:
                    self.installed_utilities[name] = utility
                    utility.installed = True
                    
            except Exception as e:
                results[name] = False
        
        return results
    
    def install_utility(self, utility: VitalUtility) -> bool:
        """
        Install a Vital-Utility.
        
        Args:
            utility: VitalUtility to install.
            
        Returns:
            True if successful, False otherwise.
        """
        try:
            # Create installation directory
            os.makedirs(self.install_dir, exist_ok=True)
            
            # Get download URL
            if not utility.download_url:
                utility.download_url = self._get_download_url(utility)
            
            if not utility.download_url:
                raise DownloadError(f"Could not determine download URL for {utility.name}")
            
            # Download the utility
            download_path = self._download_utility(utility)
            
            if not download_path:
                return False
            
            # Install the utility
            return self._install_from_download(utility, download_path)
            
        except Exception as e:
            raise InstallationError(f"Failed to install {utility.name}: {e}") from e
    
    def _get_download_url(self, utility: VitalUtility) -> Optional[str]:
        """
        Get the download URL for a Vital-Utility.
        
        Args:
            utility: VitalUtility to get URL for.
            
        Returns:
            Download URL, or None if not found.
        """
        try:
            # Try to get latest release from GitHub API
            import json
            import urllib.request
            
            # Get latest release
            api_url = f"{self.VITAL_API}/releases/latest"
            
            req = urllib.request.Request(api_url)
            req.add_header("Accept", "application/vnd.github.v3+json")
            
            with urllib.request.urlopen(req, timeout=30) as response:
                data = json.loads(response.read().decode("utf-8"))
                
                # Look for asset matching the utility
                for asset in data.get("assets", []):
                    asset_name = asset.get("name", "").lower()
                    if utility.name.lower() in asset_name:
                        return asset.get("browser_download_url")
                
                # If no specific asset found, return the first asset
                if data.get("assets"):
                    return data["assets"][0].get("browser_download_url")
            
        except Exception:
            pass
        
        return None
    
    def _download_utility(self, utility: VitalUtility) -> Optional[str]:
        """
        Download a Vital-Utility.
        
        Args:
            utility: VitalUtility to download.
            
        Returns:
            Path to downloaded file, or None if failed.
        """
        if not utility.download_url:
            return None
        
        try:
            # Create temp directory
            temp_dir = tempfile.mkdtemp()
            download_path = os.path.join(temp_dir, f"{utility.name}.zip")
            
            # Download the file
            print(f"Downloading {utility.name} from {utility.download_url}...")
            urllib.request.urlretrieve(utility.download_url, download_path)
            
            return download_path
            
        except Exception as e:
            raise DownloadError(f"Failed to download {utility.name}: {e}") from e
    
    def _install_from_download(
        self,
        utility: VitalUtility,
        download_path: str,
    ) -> bool:
        """
        Install a Vital-Utility from a downloaded file.
        
        Args:
            utility: VitalUtility to install.
            download_path: Path to downloaded file.
            
        Returns:
            True if successful, False otherwise.
        """
        try:
            import zipfile
            
            # Extract the zip file
            with zipfile.ZipFile(download_path, "r") as zip_ref:
                # Extract all files to install directory
                for file_info in zip_ref.infolist():
                    # Skip directories
                    if file_info.is_dir():
                        continue
                    
                    # Extract file
                    file_path = os.path.join(self.install_dir, file_info.filename)
                    os.makedirs(os.path.dirname(file_path), exist_ok=True)
                    
                    with zip_ref.open(file_info) as source, open(file_path, "wb") as target:
                        shutil.copyfileobj(source, target)
            
            # Clean up temp file
            os.remove(download_path)
            os.rmdir(os.path.dirname(download_path))
            
            return True
            
        except Exception as e:
            raise InstallationError(f"Failed to install from download: {e}") from e
    
    def uninstall_utility(self, name: str) -> bool:
        """
        Uninstall a Vital-Utility.
        
        Args:
            name: Name of the utility to uninstall.
            
        Returns:
            True if successful, False otherwise.
        """
        try:
            if name not in self.installed_utilities:
                return False
            
            utility = self.installed_utilities[name]
            
            # Remove utility files
            if utility.executable:
                exec_path = os.path.join(self.install_dir, utility.executable)
                if os.path.exists(exec_path):
                    os.remove(exec_path)
            
            # Remove utility directory
            utility_dir = os.path.join(self.install_dir, utility.name)
            if os.path.exists(utility_dir):
                shutil.rmtree(utility_dir)
            
            # Remove from installed list
            del self.installed_utilities[name]
            utility.installed = False
            
            return True
            
        except Exception as e:
            raise InstallationError(f"Failed to uninstall {name}: {e}") from e
    
    def update_utility(self, name: str) -> bool:
        """
        Update a Vital-Utility.
        
        Args:
            name: Name of the utility to update.
            
        Returns:
            True if successful, False otherwise.
        """
        try:
            if name not in self.installed_utilities:
                return self.install_utility(self.get_utility(name))
            
            # Uninstall first
            self.uninstall_utility(name)
            
            # Then reinstall
            return self.install_utility(self.get_utility(name))
            
        except Exception as e:
            raise InstallationError(f"Failed to update {name}: {e}") from e
    
    def get_executable_path(self, name: str) -> Optional[str]:
        """
        Get the executable path for a Vital-Utility.
        
        Args:
            name: Name of the utility.
            
        Returns:
            Path to executable, or None if not found.
        """
        if name not in self.installed_utilities:
            return None
        
        utility = self.installed_utilities[name]
        
        if utility.executable:
            path = os.path.join(self.install_dir, utility.executable)
            if os.path.exists(path):
                return path
        
        # Try common patterns
        patterns = [
            os.path.join(self.install_dir, name, f"{name}.exe"),
            os.path.join(self.install_dir, name, "bin", f"{name}.exe"),
            os.path.join(self.install_dir, f"{name}.exe"),
        ]
        
        for pattern in patterns:
            if os.path.exists(pattern):
                return pattern
        
        return None
    
    def get_replacement_for(self, windows_component: str) -> Optional[str]:
        """
        Get the Vital-Utility that replaces a Windows component.
        
        Args:
            windows_component: Name of Windows component.
            
        Returns:
            Name of Vital-Utility, or None if not found.
        """
        for name, utility in self.KNOWN_UTILITIES.items():
            if windows_component in utility.replaces:
                return name
        
        return None
    
    def get_all_replacements(self) -> Dict[str, str]:
        """
        Get all Windows component to Vital-Utility replacements.
        
        Returns:
            Dictionary mapping Windows component names to Vital-Utility names.
        """
        replacements = {}
        
        for name, utility in self.KNOWN_UTILITIES.items():
            for windows_comp in utility.replaces:
                replacements[windows_comp] = name
        
        return replacements


# Global instance
vital_manager = VitalUtilitiesManager()


def ensure_vital_utilities(
    utilities: Optional[List[str]] = None,
    all_utilities: bool = False,
    force_reinstall: bool = False,
) -> Dict[str, bool]:
    """
    Ensure Vital-Utilities are installed.
    
    Args:
        utilities: List of utility names to install.
        all_utilities: If True, install all known utilities.
        force_reinstall: If True, reinstall even if already installed.
        
    Returns:
        Dictionary of utility names and installation success status.
    """
    return vital_manager.ensure_vital_utilities(utilities, all_utilities, force_reinstall)


if __name__ == "__main__":
    # Example usage
    print("Vital-Utilities Manager")
    print("======================")
    
    manager = VitalUtilitiesManager()
    
    # List all utilities
    print("\nAvailable utilities:")
    for utility in manager.list_utilities():
        installed = "[INSTALLED]" if manager.is_installed(utility.name) else ""
        print(f"  {utility.name}: {utility.display_name} {installed}")
    
    # Show replacements
    print("\nReplacements:")
    replacements = manager.get_all_replacements()
    for windows_comp, vital_comp in replacements.items():
        print(f"  {windows_comp} -> {vital_comp}")
    
    # Ensure vital-shell is installed
    print("\nEnsuring vital-shell is installed...")
    results = manager.ensure_vital_utilities(["vital-shell"])
    for name, success in results.items():
        print(f"  {name}: {'SUCCESS' if success else 'FAILED'}")
