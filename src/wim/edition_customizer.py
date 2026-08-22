# FreeNT Edition Customizer
# Copyright (c) 2026, Panoc95
# BSD 3-Clause License

"""
Windows edition customization for FreeNT.
This module handles customizing Windows editions in WIM images,
removing unwanted editions and keeping only FreeNT editions.
"""

import os
import sys
import xml.etree.ElementTree as ET
from dataclasses import dataclass, field
from typing import Optional, List, Dict, Any, Tuple
from pathlib import Path

from .wim_manager import WIMManager, WIMError, WIMImageInfo


class EditionError(Exception):
    """Base exception for edition customization errors."""
    pass


class EditionNotFoundError(EditionError):
    """Raised when an edition is not found."""
    pass


class EditionRemoveError(EditionError):
    """Raised when edition removal fails."""
    pass


@dataclass
class WindowsEdition:
    """Represents a Windows edition."""
    
    name: str
    display_name: str
    description: str
    index: int
    product_key: Optional[str] = None
    channel: Optional[str] = None
    edition_id: Optional[str] = None
    
    def to_dict(self) -> Dict[str, Any]:
        """Convert to dictionary."""
        return {
            "name": self.name,
            "display_name": self.display_name,
            "description": self.description,
            "index": self.index,
            "product_key": self.product_key,
            "channel": self.channel,
            "edition_id": self.edition_id,
        }


@dataclass
class EditionConfig:
    """Configuration for edition customization."""
    
    # Editions to keep (by name or pattern)
    keep_editions: List[str] = field(default_factory=list)
    
    # Editions to remove (by name or pattern)
    remove_editions: List[str] = field(default_factory=list)
    
    # Whether to keep only FreeNT editions
    keep_only_freent: bool = True
    
    # Custom edition names
    custom_edition_name: str = "Windows 11 FreeNT"
    custom_edition_description: str = "FreeNT - Alternative Userland for Windows"
    
    # Product key for custom edition
    custom_product_key: Optional[str] = None
    
    def to_dict(self) -> Dict[str, Any]:
        """Convert to dictionary."""
        return {
            "keep_editions": self.keep_editions,
            "remove_editions": self.remove_editions,
            "keep_only_freent": self.keep_only_freent,
            "custom_edition_name": self.custom_edition_name,
            "custom_edition_description": self.custom_edition_description,
            "custom_product_key": self.custom_product_key,
        }


class EditionCustomizer:
    """
    Customizes Windows editions in WIM images.
    
    This class provides functionality to:
    - List available editions in a WIM
    - Remove unwanted editions
    - Create custom FreeNT editions
    - Modify edition metadata
    """
    
    # Common Windows edition names
    COMMON_EDITIONS = {
        "Windows 10 Home": ["Home", "Core", "CoreSingleLanguage"],
        "Windows 10 Pro": ["Professional", "Pro"],
        "Windows 10 Enterprise": ["Enterprise", "EnterpriseN"],
        "Windows 10 Education": ["Education", "EducationN"],
        "Windows 10 IoT": ["IoT", "IoTCore"],
        "Windows 11 Home": ["Home", "Core", "CoreSingleLanguage"],
        "Windows 11 Pro": ["Professional", "Pro"],
        "Windows 11 Enterprise": ["Enterprise", "EnterpriseN", "EnterpriseG", "EnterpriseGN"],
        "Windows 11 Education": ["Education", "EducationN"],
        "Windows 11 IoT": ["IoT", "IoTCore"],
    }
    
    # FreeNT edition names
    FREENT_EDITIONS = [
        "Windows 10 FreeNT",
        "Windows 11 FreeNT",
        "FreeNT",
    ]
    
    # Editions to remove by default (keep only Enterprise for customization)
    DEFAULT_REMOVE_EDITIONS = [
        "Home",
        "Core",
        "CoreSingleLanguage",
        "Professional",
        "Pro",
        "Education",
        "EducationN",
        "IoT",
        "IoTCore",
        "Cloud",
        "CloudN",
    ]
    
    def __init__(self, wim_manager: Optional[WIMManager] = None):
        """
        Initialize the edition customizer.
        
        Args:
            wim_manager: Optional WIMManager instance.
        """
        self.wim_manager = wim_manager or WIMManager()
        self.config = EditionConfig(
            keep_only_freent=True,
            custom_edition_name="Windows 11 FreeNT",
            custom_edition_description="FreeNT - Alternative Userland for Windows 11",
        )
    
    def list_editions(
        self,
        wim_path: str,
    ) -> List[WindowsEdition]:
        """
        List available editions in a WIM file.
        
        Args:
            wim_path: Path to WIM file.
            
        Returns:
            List of WindowsEdition objects.
        """
        try:
            # Get WIM info
            images = self.wim_manager.get_wim_info(wim_path)
            
            editions = []
            for image in images:
                edition = WindowsEdition(
                    name=image.name,
                    display_name=image.name,
                    description=image.description,
                    index=image.index,
                    product_key=None,
                    channel=None,
                    edition_id=None,
                )
                
                # Try to get more info from XML
                self._enhance_edition_info(wim_path, edition)
                
                editions.append(edition)
            
            return editions
            
        except Exception as e:
            raise EditionError(f"Failed to list editions: {e}") from e
    
    def _enhance_edition_info(
        self,
        wim_path: str,
        edition: WindowsEdition,
    ) -> None:
        """Enhance edition info with XML metadata."""
        try:
            # Try to find XML file
            xml_path = wim_path + ".xml"
            if not os.path.exists(xml_path):
                return
            
            tree = ET.parse(xml_path)
            root = tree.getroot()
            
            # Find edition info
            for elem in root.findall(".//WINDOWS"):
                if elem.get("INDEX") == str(edition.index):
                    edition.display_name = elem.get("DISPLAY_NAME", edition.display_name)
                    edition.product_key = elem.get("PRODUCT_KEY")
                    edition.channel = elem.get("CHANNEL")
                    edition.edition_id = elem.get("EDITION_ID")
                    break
            
        except Exception:
            pass
    
    def identify_enterprise_edition(
        self,
        editions: List[WindowsEdition],
    ) -> Optional[WindowsEdition]:
        """
        Identify the Enterprise edition from a list of editions.
        
        Args:
            editions: List of WindowsEdition objects.
            
        Returns:
            WindowsEdition for Enterprise, or None if not found.
        """
        enterprise_patterns = [
            "enterprise",
            "Enterprise",
            "ENTERPRISE",
        ]
        
        for edition in editions:
            for pattern in enterprise_patterns:
                if pattern in edition.name.lower():
                    return edition
        
        return None
    
    def customize_editions(
        self,
        wim_path: str,
        config: Optional[EditionConfig] = None,
        output_wim: Optional[str] = None,
    ) -> str:
        """
        Customize editions in a WIM file.
        
        This method:
        1. Identifies the Enterprise edition
        2. Removes unwanted editions
        3. Creates custom FreeNT edition based on Enterprise
        4. Saves to new WIM file
        
        Args:
            wim_path: Path to WIM file.
            config: Optional EditionConfig.
            output_wim: Optional output WIM path.
            
        Returns:
            Path to customized WIM file.
        """
        if config:
            self.config = config
        
        try:
            # Get list of editions
            editions = self.list_editions(wim_path)
            
            if not editions:
                raise EditionNotFoundError("No editions found in WIM")
            
            # Find Enterprise edition
            enterprise = self.identify_enterprise_edition(editions)
            
            if not enterprise and self.config.keep_only_freent:
                raise EditionNotFoundError("Enterprise edition not found in WIM")
            
            # Determine output path
            if output_wim is None:
                base, ext = os.path.splitext(wim_path)
                output_wim = f"{base}_freent{ext}"
            
            # If we have Enterprise, create custom edition from it
            if enterprise and self.config.keep_only_freent:
                # Mount Enterprise edition
                session = self.wim_manager.mount_wim(wim_path, enterprise.index)
                
                try:
                    # Customize the mounted image
                    self._customize_mounted_image(
                        session.mount_dir,
                        self.config.custom_edition_name,
                        self.config.custom_edition_description,
                    )
                    
                    # Unmount and commit to new WIM
                    self.wim_manager.unmount_wim(session.mount_dir, commit=True)
                    
                    # Export the customized image to new WIM
                    self.wim_manager.export_image(
                        wim_path,
                        enterprise.index,
                        output_wim,
                        dest_name=self.config.custom_edition_name,
                        compression="max",
                    )
                    
                except Exception as e:
                    self.wim_manager.unmount_wim(session.mount_dir, commit=False)
                    raise EditionError(f"Failed to customize Enterprise edition: {e}") from e
                
                # Remove other editions from new WIM
                self._remove_unwanted_editions(output_wim)
            else:
                # Just copy the WIM and remove unwanted editions
                shutil.copy2(wim_path, output_wim)
                self._remove_unwanted_editions(output_wim)
            
            return output_wim
            
        except Exception as e:
            raise EditionError(f"Failed to customize editions: {e}") from e
    
    def _customize_mounted_image(
        self,
        mount_dir: str,
        edition_name: str,
        edition_description: str,
    ) -> None:
        """
        Customize a mounted WIM image.
        
        Args:
            mount_dir: Directory where WIM is mounted.
            edition_name: New edition name.
            edition_description: New edition description.
        """
        try:
            # Modify Windows registry in mounted image
            self._modify_edition_registry(mount_dir, edition_name, edition_description)
            
            # Modify setup files
            self._modify_setup_files(mount_dir, edition_name)
            
            # Modify unattend.xml if exists
            self._modify_unattend_xml(mount_dir, edition_name)
            
        except Exception as e:
            raise EditionError(f"Failed to customize mounted image: {e}") from e
    
    def _modify_edition_registry(
        self,
        mount_dir: str,
        edition_name: str,
        edition_description: str,
    ) -> None:
        """Modify registry to change edition name."""
        try:
            import winreg
            
            # Path to registry hives in mounted image
            system_hive = os.path.join(mount_dir, "Windows", "System32", "config", "SYSTEM")
            software_hive = os.path.join(mount_dir, "Windows", "System32", "config", "SOFTWARE")
            
            # We'll use regedit to modify the offline registry
            # Create a reg file with our changes
            reg_content = f"""Windows Registry Editor Version 5.00

[HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows NT\CurrentVersion]
"ProductName"="{edition_name}"
"DisplayVersion"="10.0"
"CurrentBuild"="FreeNT"
"CurrentVersion"="{edition_name}"
"EditionID"="FreeNT"
"InstallationType"="Client"
"ProductId"="00330-80000-00000-AAOEM"

[HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion]
"ProductName"="{edition_name}"
"CurrentVersion"="{edition_name}"
"CurrentBuildNumber"="FreeNT"
"CurrentType"="Multiprocessor Free"
"EditionID"="FreeNT"

[HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\ProductOptions]
"ProductName"="{edition_name}"
"ProductPolicy"=dword:00000000
"""
            
            reg_file = os.path.join(mount_dir, "freent_edition.reg")
            with open(reg_file, "w") as f:
                f.write(reg_content)
            
            # Apply reg file to offline registry
            subprocess.run(
                ["reg", "import", reg_file],
                cwd=mount_dir,
                capture_output=True,
                timeout=30,
            )
            
            # Clean up
            os.remove(reg_file)
            
        except Exception as e:
            print(f"Warning: Failed to modify edition registry: {e}")
    
    def _modify_setup_files(
        self,
        mount_dir: str,
        edition_name: str,
    ) -> None:
        """Modify setup files to reflect new edition."""
        try:
            # Modify setup.exe config if exists
            setup_exe = os.path.join(mount_dir, "setup.exe")
            if os.path.exists(setup_exe):
                # This is a placeholder - actual modification would be more complex
                pass
            
            # Modify setup.cfg if exists
            setup_cfg = os.path.join(mount_dir, "setup.cfg")
            if os.path.exists(setup_cfg):
                with open(setup_cfg, "r") as f:
                    content = f.read()
                
                # Replace edition references
                content = content.replace("Enterprise", edition_name)
                content = content.replace("ENTERPRISE", edition_name.upper())
                
                with open(setup_cfg, "w") as f:
                    f.write(content)
            
        except Exception as e:
            print(f"Warning: Failed to modify setup files: {e}")
    
    def _modify_unattend_xml(
        self,
        mount_dir: str,
        edition_name: str,
    ) -> None:
        """Modify unattend.xml to use new edition."""
        try:
            unattend_paths = [
                os.path.join(mount_dir, "unattend.xml"),
                os.path.join(mount_dir, "Windows", "System32", "unattend.xml"),
                os.path.join(mount_dir, "Windows", "Panther", "unattend.xml"),
            ]
            
            for unattend_path in unattend_paths:
                if os.path.exists(unattend_path):
                    tree = ET.parse(unattend_path)
                    root = tree.getroot()
                    
                    # Find and modify edition-related settings
                    for elem in root.iter():
                        if elem.text and "Enterprise" in elem.text:
                            elem.text = elem.text.replace("Enterprise", edition_name)
                        
                        # Modify ProductKey if exists
                        if elem.tag.endswith("ProductKey") and elem.text:
                            if self.config.custom_product_key:
                                elem.text = self.config.custom_product_key
                    
                    tree.write(unattend_path)
            
        except Exception as e:
            print(f"Warning: Failed to modify unattend.xml: {e}")
    
    def _remove_unwanted_editions(
        self,
        wim_path: str,
    ) -> None:
        """Remove unwanted editions from WIM."""
        try:
            # Get list of editions
            editions = self.list_editions(wim_path)
            
            # Determine which editions to remove
            editions_to_remove = []
            
            for edition in editions:
                # Check if this is a FreeNT edition
                is_freent = any(
                    freent_name in edition.name 
                    for freent_name in self.FREENT_EDITIONS
                )
                
                if not is_freent:
                    # Check if we should keep it
                    keep = any(
                        pattern in edition.name 
                        for pattern in self.config.keep_editions
                    )
                    
                    if not keep:
                        editions_to_remove.append(edition)
            
            # Remove editions (in reverse order to maintain indices)
            for edition in sorted(editions_to_remove, key=lambda x: x.index, reverse=True):
                try:
                    self.wim_manager.delete_image(wim_path, edition.index)
                    print(f"Removed edition: {edition.name} (index {edition.index})")
                except Exception as e:
                    print(f"Warning: Failed to remove edition {edition.name}: {e}")
            
            # Optimize the WIM
            self.wim_manager.optimize_wim(wim_path)
            
        except Exception as e:
            raise EditionRemoveError(f"Failed to remove unwanted editions: {e}") from e
    
    def create_freent_edition(
        self,
        source_wim: str,
        source_index: int,
        output_wim: str,
        edition_name: str = "Windows 11 FreeNT",
        edition_description: str = "FreeNT - Alternative Userland for Windows 11",
    ) -> str:
        """
        Create a new FreeNT edition from a source image.
        
        Args:
            source_wim: Path to source WIM file.
            source_index: Index of source image.
            output_wim: Path for output WIM file.
            edition_name: Name for new edition.
            edition_description: Description for new edition.
            
        Returns:
            Path to created WIM file.
        """
        try:
            # Mount source image
            session = self.wim_manager.mount_wim(source_wim, source_index)
            
            try:
                # Customize the mounted image
                self._customize_mounted_image(
                    session.mount_dir,
                    edition_name,
                    edition_description,
                )
                
                # Unmount and commit
                self.wim_manager.unmount_wim(session.mount_dir, commit=True)
                
                # Export to new WIM
                self.wim_manager.export_image(
                    source_wim,
                    source_index,
                    output_wim,
                    dest_name=edition_name,
                    compression="max",
                )
                
                return output_wim
                
            except Exception as e:
                self.wim_manager.unmount_wim(session.mount_dir, commit=False)
                raise EditionError(f"Failed to create FreeNT edition: {e}") from e
        
        except Exception as e:
            raise EditionError(f"Failed to create FreeNT edition: {e}") from e
    
    def create_freent_wim_from_enterprise(
        self,
        enterprise_wim: str,
        output_wim: str,
        windows_version: int = 11,
    ) -> str:
        """
        Create a FreeNT WIM from an Enterprise WIM.
        
        This is the main method for creating a FreeNT installation WIM.
        
        Args:
            enterprise_wim: Path to Enterprise WIM file.
            output_wim: Path for output FreeNT WIM file.
            windows_version: Windows version (10 or 11).
            
        Returns:
            Path to created FreeNT WIM file.
        """
        try:
            # Set up config for this version
            if windows_version == 11:
                self.config = EditionConfig(
                    keep_only_freent=True,
                    custom_edition_name="Windows 11 FreeNT",
                    custom_edition_description="FreeNT - Alternative Userland for Windows 11",
                )
            else:
                self.config = EditionConfig(
                    keep_only_freent=True,
                    custom_edition_name="Windows 10 FreeNT",
                    custom_edition_description="FreeNT - Alternative Userland for Windows 10",
                )
            
            # Customize the Enterprise WIM
            return self.customize_editions(
                enterprise_wim,
                self.config,
                output_wim,
            )
        
        except Exception as e:
            raise EditionError(f"Failed to create FreeNT WIM: {e}") from e


# Global instance
edition_customizer = EditionCustomizer()


if __name__ == "__main__":
    # Example usage
    print("Edition Customizer Example")
    print("==========================")
    
    customizer = EditionCustomizer()
    
    # Example WIM path (change this to a real WIM path)
    wim_path = "C:\\Windows\System32\install.wim"
    
    if os.path.exists(wim_path):
        print(f"\nWIM File: {wim_path}")
        
        try:
            # List editions
            editions = customizer.list_editions(wim_path)
            print(f"\nFound {len(editions)} editions:")
            for edition in editions:
                print(f"  Index {edition.index}: {edition.name}")
                if edition.display_name:
                    print(f"    Display: {edition.display_name}")
                if edition.product_key:
                    print(f"    Product Key: {edition.product_key}")
            
            # Identify Enterprise
            enterprise = customizer.identify_enterprise_edition(editions)
            if enterprise:
                print(f"\nEnterprise edition: {enterprise.name} (index {enterprise.index})")
            else:
                print("\nNo Enterprise edition found")
            
        except Exception as e:
            print(f"Error: {e}")
    else:
        print(f"WIM file not found: {wim_path}")
