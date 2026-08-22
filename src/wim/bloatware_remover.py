# FreeNT Bloatware Remover
# Copyright (c) 2026, Panoc95
# BSD 3-Clause License

"""
Bloatware removal for FreeNT.
This module handles removing MsStore apps, unnecessary packages,
and other bloatware from Windows WIM images.
"""

import os
import sys
import subprocess
import shutil
import xml.etree.ElementTree as ET
from dataclasses import dataclass, field
from typing import Optional, List, Dict, Any, Set, Tuple
from pathlib import Path

from .wim_manager import WIMManager, WIMError


class BloatwareError(Exception):
    """Base exception for bloatware removal errors."""
    pass


class BloatwareNotFoundError(BloatwareError):
    """Raised when bloatware is not found."""
    pass


class BloatwareRemoveError(BloatwareError):
    """Raised when bloatware removal fails."""
    pass


@dataclass
class BloatwarePackage:
    """Represents a bloatware package."""
    
    name: str
    display_name: str
    description: str
    package_id: Optional[str] = None
    publisher: Optional[str] = None
    is_msstore: bool = False
    is_provisioned: bool = False
    removal_method: str = "dism"  # dism, winget, registry, file
    
    def to_dict(self) -> Dict[str, Any]:
        """Convert to dictionary."""
        return {
            "name": self.name,
            "display_name": self.display_name,
            "description": self.description,
            "package_id": self.package_id,
            "publisher": self.publisher,
            "is_msstore": self.is_msstore,
            "is_provisioned": self.is_provisioned,
            "removal_method": self.removal_method,
        }


@dataclass
class BloatwareConfig:
    """Configuration for bloatware removal."""
    
    # Categories of bloatware to remove
    remove_categories: List[str] = field(default_factory=list)
    
    # Specific packages to remove
    remove_packages: List[str] = field(default_factory=list)
    
    # Specific packages to keep (even if in remove list)
    keep_packages: List[str] = field(default_factory=list)
    
    # Whether to remove all MsStore apps
    remove_all_msstore: bool = True
    
    # Whether to remove provisioned packages
    remove_provisioned: bool = True
    
    # Whether to remove Windows capabilities
    remove_capabilities: bool = False
    
    # Aggressive removal (may break some features)
    aggressive: bool = False
    
    def to_dict(self) -> Dict[str, Any]:
        """Convert to dictionary."""
        return {
            "remove_categories": self.remove_categories,
            "remove_packages": self.remove_packages,
            "keep_packages": self.keep_packages,
            "remove_all_msstore": self.remove_all_msstore,
            "remove_provisioned": self.remove_provisioned,
            "remove_capabilities": self.remove_capabilities,
            "aggressive": self.aggressive,
        }


class BloatwareRemover:
    """
    Removes bloatware from Windows WIM images.
    
    This class provides functionality to:
    - Identify and list bloatware packages
    - Remove MsStore apps
    - Remove provisioned packages
    - Remove unnecessary Windows capabilities
    - Clean up registry entries
    """
    
    # Known bloatware packages
    KNOWN_BLOATWARE: Dict[str, BloatwarePackage] = {
        # MsStore apps
        "Microsoft.549981C3F5F10": BloatwarePackage(
            name="Cortana",
            display_name="Cortana",
            description="Virtual assistant",
            package_id="Microsoft.549981C3F5F10",
            publisher="Microsoft Corporation",
            is_msstore=True,
            is_provisioned=True,
            removal_method="dism",
        ),
        "Microsoft.BingWeather": BloatwarePackage(
            name="Bing Weather",
            display_name="Bing Weather",
            description="Weather app",
            package_id="Microsoft.BingWeather",
            publisher="Microsoft Corporation",
            is_msstore=True,
            is_provisioned=True,
            removal_method="dism",
        ),
        "Microsoft.BingNews": BloatwarePackage(
            name="Bing News",
            display_name="Bing News",
            description="News app",
            package_id="Microsoft.BingNews",
            publisher="Microsoft Corporation",
            is_msstore=True,
            is_provisioned=True,
            removal_method="dism",
        ),
        "Microsoft.BingSports": BloatwarePackage(
            name="Bing Sports",
            display_name="Bing Sports",
            description="Sports app",
            package_id="Microsoft.BingSports",
            publisher="Microsoft Corporation",
            is_msstore=True,
            is_provisioned=True,
            removal_method="dism",
        ),
        "Microsoft.BingFinance": BloatwarePackage(
            name="Bing Finance",
            display_name="Bing Finance",
            description="Finance app",
            package_id="Microsoft.BingFinance",
            publisher="Microsoft Corporation",
            is_msstore=True,
            is_provisioned=True,
            removal_method="dism",
        ),
        "Microsoft.WindowsCalculator": BloatwarePackage(
            name="Calculator",
            display_name="Calculator",
            description="Calculator app",
            package_id="Microsoft.WindowsCalculator",
            publisher="Microsoft Corporation",
            is_msstore=True,
            is_provisioned=True,
            removal_method="dism",
        ),
        "Microsoft.WindowsAlarms": BloatwarePackage(
            name="Alarms & Clock",
            display_name="Alarms & Clock",
            description="Alarms and clock app",
            package_id="Microsoft.WindowsAlarms",
            publisher="Microsoft Corporation",
            is_msstore=True,
            is_provisioned=True,
            removal_method="dism",
        ),
        "Microsoft.WindowsCamera": BloatwarePackage(
            name="Camera",
            display_name="Camera",
            description="Camera app",
            package_id="Microsoft.WindowsCamera",
            publisher="Microsoft Corporation",
            is_msstore=True,
            is_provisioned=True,
            removal_method="dism",
        ),
        "Microsoft.WindowsMaps": BloatwarePackage(
            name="Maps",
            display_name="Maps",
            description="Maps app",
            package_id="Microsoft.WindowsMaps",
            publisher="Microsoft Corporation",
            is_msstore=True,
            is_provisioned=True,
            removal_method="dism",
        ),
        "Microsoft.WindowsPhone": BloatwarePackage(
            name="Your Phone",
            display_name="Your Phone",
            description="Phone integration",
            package_id="Microsoft.WindowsPhone",
            publisher="Microsoft Corporation",
            is_msstore=True,
            is_provisioned=True,
            removal_method="dism",
        ),
        "Microsoft.WindowsFeedbackHub": BloatwarePackage(
            name="Feedback Hub",
            display_name="Feedback Hub",
            description="Feedback app",
            package_id="Microsoft.WindowsFeedbackHub",
            publisher="Microsoft Corporation",
            is_msstore=True,
            is_provisioned=True,
            removal_method="dism",
        ),
        "Microsoft.WindowsSoundRecorder": BloatwarePackage(
            name="Voice Recorder",
            display_name="Voice Recorder",
            description="Voice recording app",
            package_id="Microsoft.WindowsSoundRecorder",
            publisher="Microsoft Corporation",
            is_msstore=True,
            is_provisioned=True,
            removal_method="dism",
        ),
        "Microsoft.WindowsStore": BloatwarePackage(
            name="Microsoft Store",
            display_name="Microsoft Store",
            description="App store",
            package_id="Microsoft.WindowsStore",
            publisher="Microsoft Corporation",
            is_msstore=True,
            is_provisioned=True,
            removal_method="dism",
        ),
        "Microsoft.Office.OneNote": BloatwarePackage(
            name="OneNote",
            display_name="OneNote",
            description="Note-taking app",
            package_id="Microsoft.Office.OneNote",
            publisher="Microsoft Corporation",
            is_msstore=True,
            is_provisioned=True,
            removal_method="dism",
        ),
        "Microsoft.SkypeApp": BloatwarePackage(
            name="Skype",
            display_name="Skype",
            description="Communication app",
            package_id="Microsoft.SkypeApp",
            publisher="Microsoft Corporation",
            is_msstore=True,
            is_provisioned=True,
            removal_method="dism",
        ),
        "Microsoft.XboxIdentityProvider": BloatwarePackage(
            name="Xbox Identity Provider",
            display_name="Xbox Identity Provider",
            description="Xbox authentication",
            package_id="Microsoft.XboxIdentityProvider",
            publisher="Microsoft Corporation",
            is_msstore=True,
            is_provisioned=True,
            removal_method="dism",
        ),
        "Microsoft.Xbox.TCUI": BloatwarePackage(
            name="Xbox TCUI",
            display_name="Xbox TCUI",
            description="Xbox UI",
            package_id="Microsoft.Xbox.TCUI",
            publisher="Microsoft Corporation",
            is_msstore=True,
            is_provisioned=True,
            removal_method="dism",
        ),
        "Microsoft.XboxGameOverlay": BloatwarePackage(
            name="Xbox Game Overlay",
            display_name="Xbox Game Overlay",
            description="Xbox game overlay",
            package_id="Microsoft.XboxGameOverlay",
            publisher="Microsoft Corporation",
            is_msstore=True,
            is_provisioned=True,
            removal_method="dism",
        ),
        "Microsoft.XboxGamingOverlay": BloatwarePackage(
            name="Xbox Gaming Overlay",
            display_name="Xbox Gaming Overlay",
            description="Xbox gaming overlay",
            package_id="Microsoft.XboxGamingOverlay",
            publisher="Microsoft Corporation",
            is_msstore=True,
            is_provisioned=True,
            removal_method="dism",
        ),
        "Microsoft.XboxSpeechToTextOverlay": BloatwarePackage(
            name="Xbox Speech To Text Overlay",
            display_name="Xbox Speech To Text Overlay",
            description="Xbox speech to text",
            package_id="Microsoft.XboxSpeechToTextOverlay",
            publisher="Microsoft Corporation",
            is_msstore=True,
            is_provisioned=True,
            removal_method="dism",
        ),
        # Windows capabilities
        "Windows-Defender-ApplicationGuard": BloatwarePackage(
            name="Windows Defender Application Guard",
            display_name="Application Guard",
            description="Virtualized browser isolation",
            package_id="Windows-Defender-ApplicationGuard-Package",
            publisher="Microsoft Corporation",
            is_msstore=False,
            is_provisioned=False,
            removal_method="dism",
        ),
        "Windows-Printing-PrintToPDFServices": BloatwarePackage(
            name="Print to PDF",
            display_name="Print to PDF",
            description="Print to PDF service",
            package_id="Windows-Printing-PrintToPDFServices-Package",
            publisher="Microsoft Corporation",
            is_msstore=False,
            is_provisioned=False,
            removal_method="dism",
        ),
    }
    
    # Categories of bloatware
    BLOATWARE_CATEGORIES: Dict[str, List[str]] = {
        "msstore": [
            "Microsoft.549981C3F5F10",  # Cortana
            "Microsoft.BingWeather",
            "Microsoft.BingNews",
            "Microsoft.BingSports",
            "Microsoft.BingFinance",
            "Microsoft.WindowsCalculator",
            "Microsoft.WindowsAlarms",
            "Microsoft.WindowsCamera",
            "Microsoft.WindowsMaps",
            "Microsoft.WindowsPhone",
            "Microsoft.WindowsFeedbackHub",
            "Microsoft.WindowsSoundRecorder",
            "Microsoft.WindowsStore",
            "Microsoft.Office.OneNote",
            "Microsoft.SkypeApp",
            "Microsoft.XboxIdentityProvider",
            "Microsoft.Xbox.TCUI",
            "Microsoft.XboxGameOverlay",
            "Microsoft.XboxGamingOverlay",
            "Microsoft.XboxSpeechToTextOverlay",
        ],
        "xbox": [
            "Microsoft.XboxIdentityProvider",
            "Microsoft.Xbox.TCUI",
            "Microsoft.XboxGameOverlay",
            "Microsoft.XboxGamingOverlay",
            "Microsoft.XboxSpeechToTextOverlay",
        ],
        "bing": [
            "Microsoft.BingWeather",
            "Microsoft.BingNews",
            "Microsoft.BingSports",
            "Microsoft.BingFinance",
        ],
        "gaming": [
            "Microsoft.XboxIdentityProvider",
            "Microsoft.Xbox.TCUI",
            "Microsoft.XboxGameOverlay",
            "Microsoft.XboxGamingOverlay",
            "Microsoft.XboxSpeechToTextOverlay",
        ],
        "office": [
            "Microsoft.Office.OneNote",
        ],
        "communications": [
            "Microsoft.SkypeApp",
        ],
        "all": list(KNOWN_BLOATWARE.keys()),
    }
    
    # Packages to keep by default
    DEFAULT_KEEP_PACKAGES: List[str] = [
        # Keep these essential packages
        "Microsoft.Windows.Cortana",  # Keep for now (can be removed later)
    ]
    
    def __init__(self, wim_manager: Optional[WIMManager] = None):
        """
        Initialize the bloatware remover.
        
        Args:
            wim_manager: Optional WIMManager instance.
        """
        self.wim_manager = wim_manager or WIMManager()
        self.config = BloatwareConfig(
            remove_categories=["all"],
            remove_all_msstore=True,
            remove_provisioned=True,
            aggressive=False,
        )
    
    def list_bloatware(
        self,
        mount_dir: str,
    ) -> List[BloatwarePackage]:
        """
        List bloatware packages in a mounted WIM.
        
        Args:
            mount_dir: Directory where WIM is mounted.
            
        Returns:
            List of BloatwarePackage objects found.
        """
        try:
            # Get list of installed packages
            packages = self._get_installed_packages(mount_dir)
            
            # Identify bloatware
            bloatware = []
            for package_name, package_info in packages.items():
                if package_name in self.KNOWN_BLOATWARE:
                    bloatware.append(self.KNOWN_BLOATWARE[package_name])
                elif self._is_likely_bloatware(package_name, package_info):
                    bloatware.append(BloatwarePackage(
                        name=package_name,
                        display_name=package_info.get("DisplayName", package_name),
                        description=package_info.get("Description", ""),
                        package_id=package_name,
                        publisher=package_info.get("Publisher", ""),
                        is_msstore=self._is_msstore_package(package_name),
                        is_provisioned=self._is_provisioned_package(mount_dir, package_name),
                        removal_method="dism",
                    ))
            
            return bloatware
            
        except Exception as e:
            raise BloatwareError(f"Failed to list bloatware: {e}") from e
    
    def _get_installed_packages(
        self,
        mount_dir: str,
    ) -> Dict[str, Dict[str, str]]:
        """Get list of installed packages in mounted WIM."""
        packages = {}
        
        try:
            # Use DISM to get package list
            result = subprocess.run(
                ["dism", "/Image:" + mount_dir, "/Get-Packages"],
                capture_output=True,
                text=True,
                timeout=60,
            )
            
            if result.returncode != 0:
                return packages
            
            # Parse output
            current_package = None
            for line in result.stdout.split('\n'):
                line = line.strip()
                
                if line.startswith("Package Identity :"):
                    package_id = line.split(":", 1)[1].strip()
                    current_package = {"PackageIdentity": package_id}
                    packages[package_id] = current_package
                elif current_package and ":" in line:
                    key, value = line.split(":", 1)
                    current_package[key.strip()] = value.strip()
            
        except Exception:
            pass
        
        return packages
    
    def _is_likely_bloatware(
        self,
        package_name: str,
        package_info: Dict[str, str],
    ) -> bool:
        """Check if a package is likely bloatware."""
        # Check publisher
        publisher = package_info.get("Publisher", "").lower()
        if publisher != "microsoft corporation" and publisher != "microsoft windows":
            return True
        
        # Check for common bloatware patterns
        bloatware_patterns = [
            "bing",
            "xbox",
            "skype",
            "onenote",
            "cortana",
            "feedback",
            "camera",
            "maps",
            "alarms",
            "calculator",
            "soundrecorder",
            "phone",
            "store",
            "solitaire",
            "mine",
            "photos",
            "music",
            "video",
            "movies",
            "tv",
            "news",
            "sports",
            "weather",
            "finance",
            "game",
            "mixedreality",
            "hololens",
            "edge",
            "ie",
            "internet",
        ]
        
        package_lower = package_name.lower()
        for pattern in bloatware_patterns:
            if pattern in package_lower:
                return True
        
        return False
    
    def _is_msstore_package(self, package_name: str) -> bool:
        """Check if a package is a Microsoft Store package."""
        return (package_name.startswith("Microsoft.") or 
                package_name.startswith("ms-") or
                package_name.endswith("_x64__8wekyb3d8bbwe") or
                package_name.endswith("_x86__8wekyb3d8bbwe") or
                package_name.endswith("_arm64__8wekyb3d8bbwe"))
    
    def _is_provisioned_package(
        self,
        mount_dir: str,
        package_name: str,
    ) -> bool:
        """Check if a package is provisioned."""
        try:
            result = subprocess.run(
                ["dism", "/Image:" + mount_dir, "/Get-ProvisionedAppxPackages"],
                capture_output=True,
                text=True,
                timeout=30,
            )
            
            if result.returncode != 0:
                return False
            
            return package_name in result.stdout
            
        except Exception:
            return False
    
    def remove_bloatware(
        self,
        mount_dir: str,
        config: Optional[BloatwareConfig] = None,
    ) -> Dict[str, Any]:
        """
        Remove bloatware from a mounted WIM.
        
        Args:
            mount_dir: Directory where WIM is mounted.
            config: Optional BloatwareConfig.
            
        Returns:
            Dictionary with removal results.
        """
        if config:
            self.config = config
        
        results = {
            "removed": [],
            "failed": [],
            "skipped": [],
        }
        
        try:
            # Get list of bloatware
            bloatware = self.list_bloatware(mount_dir)
            
            # Determine which packages to remove
            packages_to_remove = []
            for package in bloatware:
                # Check if in keep list
                if package.name in self.config.keep_packages or \
                   package.package_id in self.config.keep_packages:
                    results["skipped"].append(package.name)
                    continue
                
                # Check category
                if self.config.remove_categories:
                    for category in self.config.remove_categories:
                        if category in self.BLOATWARE_CATEGORIES:
                            if package.name in self.BLOATWARE_CATEGORIES[category]:
                                packages_to_remove.append(package)
                                break
                
                # Check if all MsStore should be removed
                if self.config.remove_all_msstore and package.is_msstore:
                    packages_to_remove.append(package)
                
                # Check if provisioned should be removed
                if self.config.remove_provisioned and package.is_provisioned:
                    packages_to_remove.append(package)
            
            # Remove packages
            for package in packages_to_remove:
                try:
                    success = self._remove_package(mount_dir, package)
                    if success:
                        results["removed"].append(package.name)
                    else:
                        results["failed"].append(package.name)
                except Exception as e:
                    results["failed"].append(package.name)
                    print(f"Warning: Failed to remove {package.name}: {e}")
            
            # Remove Windows capabilities if enabled
            if self.config.remove_capabilities:
                self._remove_capabilities(mount_dir)
            
            # Clean up registry
            self._cleanup_registry(mount_dir)
            
            return results
            
        except Exception as e:
            raise BloatwareRemoveError(f"Failed to remove bloatware: {e}") from e
    
    def _remove_package(
        self,
        mount_dir: str,
        package: BloatwarePackage,
    ) -> bool:
        """Remove a single package."""
        try:
            # Try DISM first
            if package.removal_method == "dism" or True:  # Always try DISM first
                result = subprocess.run(
                    ["dism", "/Image:" + mount_dir, "/Remove-Package", 
                     "/PackageName:" + package.package_id, "/NoRestart"],
                    capture_output=True,
                    text=True,
                    timeout=60,
                )
                
                if result.returncode == 0:
                    return True
            
            # Try removing provisioned package
            if package.is_provisioned:
                result = subprocess.run(
                    ["dism", "/Image:" + mount_dir, "/Remove-ProvisionedAppxPackage",
                     "/PackageName:" + package.package_id, "/NoRestart"],
                    capture_output=True,
                    text=True,
                    timeout=60,
                )
                
                if result.returncode == 0:
                    return True
            
            # Try removing via directory
            if self._remove_package_directory(mount_dir, package):
                return True
            
            return False
            
        except Exception:
            return False
    
    def _remove_package_directory(
        self,
        mount_dir: str,
        package: BloatwarePackage,
    ) -> bool:
        """Remove package directory from mounted WIM."""
        try:
            # Find package directory
            program_files = os.path.join(mount_dir, "Program Files")
            windows_apps = os.path.join(mount_dir, "Program Files", "WindowsApps")
            
            # Search for package directory
            for root, dirs, files in os.walk(program_files):
                for dir_name in dirs:
                    if package.package_id in dir_name or package.name in dir_name:
                        package_dir = os.path.join(root, dir_name)
                        try:
                            shutil.rmtree(package_dir)
                            return True
                        except Exception:
                            pass
            
            return False
            
        except Exception:
            return False
    
    def _remove_capabilities(
        self,
        mount_dir: str,
    ) -> None:
        """Remove unnecessary Windows capabilities."""
        try:
            # List of capabilities to remove
            capabilities_to_remove = [
                "Windows-Defender-ApplicationGuard",
                "Windows-Printing-PrintToPDFServices",
            ]
            
            for capability in capabilities_to_remove:
                try:
                    subprocess.run(
                        ["dism", "/Image:" + mount_dir, "/Disable-Feature",
                         "/FeatureName:" + capability, "/NoRestart"],
                        capture_output=True,
                        text=True,
                        timeout=30,
                    )
                except Exception:
                    pass
            
        except Exception as e:
            print(f"Warning: Failed to remove capabilities: {e}")
    
    def _cleanup_registry(
        self,
        mount_dir: str,
    ) -> None:
        """Clean up registry entries for removed bloatware."""
        try:
            # Remove MsStore registry entries
            reg_paths = [
                "HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Appx\AppxAllUserStore",
                "HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Appx\AppxAllUserStore\Deprovisioned",
            ]
            
            for reg_path in reg_paths:
                try:
                    subprocess.run(
                        ["reg", "delete", reg_path, "/f"],
                        cwd=mount_dir,
                        capture_output=True,
                        timeout=10,
                    )
                except Exception:
                    pass
            
        except Exception as e:
            print(f"Warning: Failed to cleanup registry: {e}")
    
    def remove_all_msstore_apps(
        self,
        mount_dir: str,
    ) -> Dict[str, Any]:
        """
        Remove all Microsoft Store apps from mounted WIM.
        
        Args:
            mount_dir: Directory where WIM is mounted.
            
        Returns:
            Dictionary with removal results.
        """
        config = BloatwareConfig(
            remove_all_msstore=True,
            remove_provisioned=True,
            aggressive=True,
        )
        
        return self.remove_bloatware(mount_dir, config)
    
    def remove_common_bloatware(
        self,
        mount_dir: str,
    ) -> Dict[str, Any]:
        """
        Remove common bloatware (non-aggressive).
        
        Args:
            mount_dir: Directory where WIM is mounted.
            
        Returns:
            Dictionary with removal results.
        """
        config = BloatwareConfig(
            remove_categories=["bing", "xbox", "gaming", "office", "communications"],
            remove_all_msstore=False,
            remove_provisioned=True,
            aggressive=False,
        )
        
        return self.remove_bloatware(mount_dir, config)
    
    def cleanup_wim(
        self,
        wim_path: str,
        image_index: int,
        aggressive: bool = False,
    ) -> Dict[str, Any]:
        """
        Clean up a WIM image by removing bloatware.
        
        Args:
            wim_path: Path to WIM file.
            image_index: Index of image to clean up.
            aggressive: Whether to use aggressive cleanup.
            
        Returns:
            Dictionary with cleanup results.
        """
        try:
            # Mount the WIM
            session = self.wim_manager.mount_wim(wim_path, image_index)
            
            try:
                # Remove bloatware
                if aggressive:
                    results = self.remove_all_msstore_apps(session.mount_dir)
                else:
                    results = self.remove_common_bloatware(session.mount_dir)
                
                # Commit changes
                self.wim_manager.unmount_wim(session.mount_dir, commit=True)
                
                return results
                
            except Exception as e:
                self.wim_manager.unmount_wim(session.mount_dir, commit=False)
                raise BloatwareRemoveError(f"Failed to cleanup WIM: {e}") from e
        
        except Exception as e:
            raise BloatwareRemoveError(f"Failed to cleanup WIM: {e}") from e


# Global instance
bloatware_remover = BloatwareRemover()


if __name__ == "__main__":
    # Example usage
    print("Bloatware Remover Example")
    print("========================")
    
    remover = BloatwareRemover()
    
    # Example: This would be used on a mounted WIM
    print("\nTo use the bloatware remover:")
    print("1. Mount a WIM image using WIMManager")
    print("2. Call remover.remove_bloatware(mount_dir)")
    print("3. Commit changes and unmount")
    print("\nKnown bloatware packages:")
    for name, package in list(remover.KNOWN_BLOATWARE.items())[:10]:
        print(f"  - {package.display_name} ({name})")
