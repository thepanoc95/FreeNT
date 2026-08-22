# FreeNT WIM Manager
# Copyright (c) 2026, Panoc95
# BSD 3-Clause License

"""
WIM (Windows Imaging Format) management for FreeNT.
This module provides functionality to mount, modify, and unmount WIM images.
"""

import os
import sys
import subprocess
import tempfile
import shutil
import time
from dataclasses import dataclass, field
from typing import Optional, List, Dict, Any, Union, Tuple
from pathlib import Path
import xml.etree.ElementTree as ET


class WIMError(Exception):
    """Base exception for WIM operations."""
    pass


class WIMMountError(WIMError):
    """Raised when WIM mount fails."""
    pass


class WIMUnmountError(WIMError):
    """Raised when WIM unmount fails."""
    pass


class WIMInfoError(WIMError):
    """Raised when WIM info retrieval fails."""
    pass


class WIMCommitError(WIMError):
    """Raised when WIM commit fails."""
    pass


@dataclass
class WIMImageInfo:
    """Information about a WIM image."""
    
    index: int
    name: str
    description: str
    size: int
    path: str
    windows_version: str
    windows_edition: str
    architecture: str
    build: str
    lang: str
    
    def to_dict(self) -> Dict[str, Any]:
        """Convert to dictionary."""
        return {
            "index": self.index,
            "name": self.name,
            "description": self.description,
            "size": self.size,
            "path": self.path,
            "windows_version": self.windows_version,
            "windows_edition": self.windows_edition,
            "architecture": self.architecture,
            "build": self.build,
            "lang": self.lang,
        }


@dataclass
class WIMSession:
    """Represents a mounted WIM session."""
    
    wim_path: str
    mount_dir: str
    image_index: int
    read_only: bool = False
    
    def to_dict(self) -> Dict[str, Any]:
        """Convert to dictionary."""
        return {
            "wim_path": self.wim_path,
            "mount_dir": self.mount_dir,
            "image_index": self.image_index,
            "read_only": self.read_only,
        }


@dataclass
class WIMManagerConfig:
    """Configuration for WIM manager."""
    
    # DISM path (can be customized)
    dism_path: str = "dism"
    
    # ImageX path (alternative)
    imagex_path: str = "imagex"
    
    # Temp directory for mounting
    temp_dir: Optional[str] = None
    
    # Maximum retries for operations
    max_retries: int = 3
    
    # Timeout for operations (seconds)
    timeout: int = 300
    
    # Verbose output
    verbose: bool = True
    
    def __post_init__(self):
        """Initialize defaults."""
        if self.temp_dir is None:
            self.temp_dir = tempfile.mkdtemp(prefix="freent_wim_")


class WIMManager:
    """
    Manages WIM (Windows Imaging Format) images.
    
    This class provides functionality to:
    - List images in a WIM file
    - Mount WIM images
    - Unmount WIM images
    - Modify mounted images
    - Commit changes
    - Create new WIM files
    """
    
    def __init__(self, config: Optional[WIMManagerConfig] = None):
        """
        Initialize the WIM manager.
        
        Args:
            config: Optional WIMManagerConfig.
        """
        self.config = config or WIMManagerConfig()
        self._mounted_sessions: Dict[str, WIMSession] = {}
    
    def _run_dism(self, args: List[str], check: bool = True) -> subprocess.CompletedProcess[str]:
        """
        Run DISM command.
        
        Args:
            args: Arguments to pass to DISM.
            check: Whether to raise exception on non-zero exit.
            
        Returns:
            CompletedProcess with command results.
        """
        cmd = [self.config.dism_path] + args
        
        try:
            result = subprocess.run(
                cmd,
                capture_output=True,
                text=True,
                timeout=self.config.timeout,
                check=check,
            )
            
            if self.config.verbose:
                if result.stdout:
                    print(result.stdout)
                if result.stderr:
                    print(result.stderr, file=sys.stderr)
            
            return result
            
        except subprocess.TimeoutExpired as e:
            raise WIMError(f"DISM command timed out: {' '.join(cmd)}") from e
        except subprocess.CalledProcessError as e:
            if check:
                raise WIMError(f"DISM command failed: {' '.join(cmd)}\n{e.stderr}") from e
            return e.process
    
    def _run_imagex(self, args: List[str], check: bool = True) -> subprocess.CompletedProcess[str]:
        """
        Run ImageX command.
        
        Args:
            args: Arguments to pass to ImageX.
            check: Whether to raise exception on non-zero exit.
            
        Returns:
            CompletedProcess with command results.
        """
        cmd = [self.config.imagex_path] + args
        
        try:
            result = subprocess.run(
                cmd,
                capture_output=True,
                text=True,
                timeout=self.config.timeout,
                check=check,
            )
            
            if self.config.verbose:
                if result.stdout:
                    print(result.stdout)
                if result.stderr:
                    print(result.stderr, file=sys.stderr)
            
            return result
            
        except subprocess.TimeoutExpired as e:
            raise WIMError(f"ImageX command timed out: {' '.join(cmd)}") from e
        except subprocess.CalledProcessError as e:
            if check:
                raise WIMError(f"ImageX command failed: {' '.join(cmd)}\n{e.stderr}") from e
            return e.process
    
    def get_wim_info(self, wim_path: str) -> List[WIMImageInfo]:
        """
        Get information about images in a WIM file.
        
        Args:
            wim_path: Path to WIM file.
            
        Returns:
            List of WIMImageInfo objects.
        """
        try:
            # Use DISM to get WIM info
            result = self._run_dism(["/Get-WimInfo", "/WimFile:" + wim_path])
            
            # Parse the output
            images = []
            current_index = None
            current_info = {}
            
            for line in result.stdout.split('\n'):
                line = line.strip()
                
                if line.startswith("Index:"):
                    if current_index is not None:
                        images.append(self._parse_image_info(current_index, current_info))
                    current_index = int(line.split(":")[1].strip())
                    current_info = {}
                elif line.startswith("Name:"):
                    current_info["name"] = line.split(":", 1)[1].strip()
                elif line.startswith("Description:"):
                    current_info["description"] = line.split(":", 1)[1].strip()
                elif line.startswith("Size:"):
                    current_info["size"] = int(line.split(":")[1].strip().split()[0])
            
            # Add the last image
            if current_index is not None:
                images.append(self._parse_image_info(current_index, current_info))
            
            # Get additional info from XML if available
            self._enhance_with_xml(wim_path, images)
            
            return images
            
        except Exception as e:
            raise WIMInfoError(f"Failed to get WIM info: {e}") from e
    
    def _parse_image_info(self, index: int, info: Dict[str, Any]) -> WIMImageInfo:
        """Parse image info into WIMImageInfo object."""
        return WIMImageInfo(
            index=index,
            name=info.get("name", f"Image {index}"),
            description=info.get("description", ""),
            size=info.get("size", 0),
            path="",
            windows_version="",
            windows_edition="",
            architecture="",
            build="",
            lang="",
        )
    
    def _enhance_with_xml(self, wim_path: str, images: List[WIMImageInfo]) -> None:
        """Enhance image info with XML metadata."""
        try:
            # Try to find XML file
            xml_path = wim_path + ".xml"
            if not os.path.exists(xml_path):
                return
            
            tree = ET.parse(xml_path)
            root = tree.getroot()
            
            # Find Windows info
            for image in images:
                for elem in root.findall(".//WINDOWS"):
                    if elem.get("INDEX") == str(image.index):
                        image.windows_version = elem.get("VERSION", "")
                        image.windows_edition = elem.get("EDITION", "")
                        image.architecture = elem.get("ARCH", "")
                        image.build = elem.get("BUILD", "")
                        image.lang = elem.get("LANG", "")
                        break
            
        except Exception:
            pass
    
    def mount_wim(
        self,
        wim_path: str,
        image_index: int,
        mount_dir: Optional[str] = None,
        read_only: bool = False,
    ) -> WIMSession:
        """
        Mount a WIM image.
        
        Args:
            wim_path: Path to WIM file.
            image_index: Index of image to mount.
            mount_dir: Directory to mount to. If None, creates temp directory.
            read_only: Whether to mount read-only.
            
        Returns:
            WIMSession object.
        """
        try:
            # Create mount directory if not provided
            if mount_dir is None:
                mount_dir = tempfile.mkdtemp(prefix=f"freent_mount_{image_index}_")
            else:
                os.makedirs(mount_dir, exist_ok=True)
            
            # Check if already mounted
            if mount_dir in self._mounted_sessions:
                raise WIMMountError(f"Already mounted at {mount_dir}")
            
            # Build DISM command
            cmd = [
                "/Mount-Wim",
                f"/WimFile:{wim_path}",
                f"/Index:{image_index}",
                f"/MountDir:{mount_dir}",
            ]
            
            if read_only:
                cmd.append("/ReadOnly")
            
            # Execute DISM
            result = self._run_dism(cmd)
            
            if result.returncode != 0:
                raise WIMMountError(f"Failed to mount WIM: {result.stderr}")
            
            # Create session
            session = WIMSession(
                wim_path=wim_path,
                mount_dir=mount_dir,
                image_index=image_index,
                read_only=read_only,
            )
            
            self._mounted_sessions[mount_dir] = session
            
            return session
            
        except Exception as e:
            # Clean up mount directory if created
            if mount_dir and os.path.exists(mount_dir):
                try:
                    shutil.rmtree(mount_dir)
                except Exception:
                    pass
            raise WIMMountError(f"Failed to mount WIM: {e}") from e
    
    def unmount_wim(
        self,
        mount_dir: str,
        commit: bool = True,
        check_integrity: bool = True,
    ) -> bool:
        """
        Unmount a WIM image.
        
        Args:
            mount_dir: Directory where WIM is mounted.
            commit: Whether to commit changes.
            check_integrity: Whether to check integrity.
            
        Returns:
            True if successful, False otherwise.
        """
        try:
            # Check if mounted
            if mount_dir not in self._mounted_sessions:
                raise WIMUnmountError(f"Not mounted at {mount_dir}")
            
            session = self._mounted_sessions[mount_dir]
            
            # Build DISM command
            cmd = [
                "/Unmount-Wim",
                f"/MountDir:{mount_dir}",
            ]
            
            if commit:
                cmd.append("/Commit")
            else:
                cmd.append("/Discard")
            
            if check_integrity:
                cmd.append("/CheckIntegrity")
            
            # Execute DISM
            result = self._run_dism(cmd)
            
            if result.returncode != 0:
                raise WIMUnmountError(f"Failed to unmount WIM: {result.stderr}")
            
            # Remove from sessions
            del self._mounted_sessions[mount_dir]
            
            # Clean up mount directory
            if os.path.exists(mount_dir):
                try:
                    shutil.rmtree(mount_dir)
                except Exception:
                    pass
            
            return True
            
        except Exception as e:
            raise WIMUnmountError(f"Failed to unmount WIM: {e}") from e
    
    def commit_wim(
        self,
        mount_dir: str,
        new_wim_path: Optional[str] = None,
        compression: str = "max",
    ) -> bool:
        """
        Commit changes to a mounted WIM image.
        
        Args:
            mount_dir: Directory where WIM is mounted.
            new_wim_path: Optional path for new WIM file. If None, updates original.
            compression: Compression type (none, fast, max).
            
        Returns:
            True if successful, False otherwise.
        """
        try:
            # Check if mounted
            if mount_dir not in self._mounted_sessions:
                raise WIMCommitError(f"Not mounted at {mount_dir}")
            
            session = self._mounted_sessions[mount_dir]
            
            # First unmount with commit
            self.unmount_wim(mount_dir, commit=True)
            
            # If new path specified, export to new file
            if new_wim_path:
                self.export_image(
                    session.wim_path,
                    session.image_index,
                    new_wim_path,
                    compression=compression,
                )
            
            return True
            
        except Exception as e:
            raise WIMCommitError(f"Failed to commit WIM: {e}") from e
    
    def export_image(
        self,
        source_wim: str,
        source_index: int,
        dest_wim: str,
        dest_name: Optional[str] = None,
        compression: str = "max",
        reference_wims: Optional[List[str]] = None,
    ) -> bool:
        """
        Export an image from one WIM to another.
        
        Args:
            source_wim: Source WIM file.
            source_index: Index of image to export.
            dest_wim: Destination WIM file.
            dest_name: Optional name for exported image.
            compression: Compression type (none, fast, max).
            reference_wims: Optional list of reference WIMs.
            
        Returns:
            True if successful, False otherwise.
        """
        try:
            cmd = [
                "/Export-Image",
                f"/SourceImageFile:{source_wim}",
                f"/SourceIndex:{source_index}",
                f"/DestinationImageFile:{dest_wim}",
                f"/Compress:{compression}",
            ]
            
            if dest_name:
                cmd.append(f"/DestinationName:{dest_name}")
            
            if reference_wims:
                for ref_wim in reference_wims:
                    cmd.append(f"/Ref:{ref_wim}")
            
            result = self._run_dism(cmd)
            
            if result.returncode != 0:
                raise WIMError(f"Failed to export image: {result.stderr}")
            
            return True
            
        except Exception as e:
            raise WIMError(f"Failed to export image: {e}") from e
    
    def add_image(
        self,
        wim_path: str,
        source_dir: str,
        name: str,
        description: str = "",
        compression: str = "max",
        config_file: Optional[str] = None,
    ) -> bool:
        """
        Add a new image to a WIM file.
        
        Args:
            wim_path: Path to WIM file.
            source_dir: Directory to add as image.
            name: Name for the new image.
            description: Description for the new image.
            compression: Compression type (none, fast, max).
            config_file: Optional config file.
            
        Returns:
            True if successful, False otherwise.
        """
        try:
            cmd = [
                "/Add-Image",
                f"/ImageFile:{wim_path}",
                f"/CaptureDir:{source_dir}",
                f"/Name:{name}",
                f"/Compress:{compression}",
            ]
            
            if description:
                cmd.append(f"/Description:{description}")
            
            if config_file:
                cmd.append(f"/ConfigFile:{config_file}")
            
            result = self._run_dism(cmd)
            
            if result.returncode != 0:
                raise WIMError(f"Failed to add image: {result.stderr}")
            
            return True
            
        except Exception as e:
            raise WIMError(f"Failed to add image: {e}") from e
    
    def delete_image(
        self,
        wim_path: str,
        image_index: int,
    ) -> bool:
        """
        Delete an image from a WIM file.
        
        Args:
            wim_path: Path to WIM file.
            image_index: Index of image to delete.
            
        Returns:
            True if successful, False otherwise.
        """
        try:
            cmd = [
                "/Delete-Image",
                f"/ImageFile:{wim_path}",
                f"/Index:{image_index}",
            ]
            
            result = self._run_dism(cmd)
            
            if result.returncode != 0:
                raise WIMError(f"Failed to delete image: {result.stderr}")
            
            return True
            
        except Exception as e:
            raise WIMError(f"Failed to delete image: {e}") from e
    
    def optimize_wim(
        self,
        wim_path: str,
        output_path: Optional[str] = None,
    ) -> bool:
        """
        Optimize a WIM file by removing duplicate files.
        
        Args:
            wim_path: Path to WIM file.
            output_path: Optional output path. If None, overwrites original.
            
        Returns:
            True if successful, False otherwise.
        """
        try:
            cmd = [
                "/Optimize-Wim",
                f"/ImageFile:{wim_path}",
            ]
            
            if output_path:
                cmd.append(f"/OutputFile:{output_path}")
            
            result = self._run_dism(cmd)
            
            if result.returncode != 0:
                raise WIMError(f"Failed to optimize WIM: {result.stderr}")
            
            return True
            
        except Exception as e:
            raise WIMError(f"Failed to optimize WIM: {e}") from e
    
    def cleanup(self) -> None:
        """Clean up all mounted sessions."""
        for mount_dir, session in list(self._mounted_sessions.items()):
            try:
                self.unmount_wim(mount_dir, commit=False)
            except Exception as e:
                print(f"Warning: Failed to clean up {mount_dir}: {e}")
        
        self._mounted_sessions.clear()
    
    def get_mounted_sessions(self) -> List[WIMSession]:
        """Get list of currently mounted sessions."""
        return list(self._mounted_sessions.values())
    
    def is_mounted(self, mount_dir: str) -> bool:
        """Check if a directory has a mounted WIM."""
        return mount_dir in self._mounted_sessions


# Global instance
wim_manager = WIMManager()


if __name__ == "__main__":
    # Example usage
    print("WIM Manager Example")
    print("===================")
    
    manager = WIMManager()
    
    # List available WIM files
    wim_files = [
        "C:\\Windows\System32\install.wim",
        "C:\\Windows\System32\boot.wim",
    ]
    
    for wim_file in wim_files:
        if os.path.exists(wim_file):
            print(f"\nWIM File: {wim_file}")
            try:
                images = manager.get_wim_info(wim_file)
                for image in images:
                    print(f"  Index {image.index}: {image.name} ({image.description})")
                    print(f"    Size: {image.size} bytes")
                    if image.windows_version:
                        print(f"    Windows: {image.windows_version} {image.windows_edition} ({image.architecture})")
            except Exception as e:
                print(f"    Error: {e}")
