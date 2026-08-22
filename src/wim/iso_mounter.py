# FreeNT ISO Mounter
# Copyright (c) 2026, Panoc95
# BSD 3-Clause License

"""
ISO mounting functionality for FreeNT.
This module provides functionality to mount and extract Windows ISO files.
"""

import os
import sys
import subprocess
import tempfile
import shutil
import time
from dataclasses import dataclass, field
from typing import Optional, List, Dict, Any, Union
from pathlib import Path


class ISOError(Exception):
    """Base exception for ISO operations."""
    pass


class ISOMountError(ISOError):
    """Raised when ISO mount fails."""
    pass


class ISOExtractError(ISOError):
    """Raised when ISO extraction fails."""
    pass


@dataclass
class ISOMountInfo:
    """Information about a mounted ISO."""
    
    iso_path: str
    mount_dir: str
    drive_letter: Optional[str] = None
    virtual: bool = False
    
    def to_dict(self) -> Dict[str, Any]:
        """Convert to dictionary."""
        return {
            "iso_path": self.iso_path,
            "mount_dir": self.mount_dir,
            "drive_letter": self.drive_letter,
            "virtual": self.virtual,
        }


@dataclass
class ISOFileInfo:
    """Information about files in an ISO."""
    
    name: str
    path: str
    size: int
    is_directory: bool
    
    def to_dict(self) -> Dict[str, Any]:
        """Convert to dictionary."""
        return {
            "name": self.name,
            "path": self.path,
            "size": self.size,
            "is_directory": self.is_directory,
        }


class ISOMounter:
    """
    Manages ISO mounting and extraction.
    
    This class provides functionality to:
    - Mount ISO files (Windows only)
    - Extract ISO files
    - List files in ISO
    - Find WIM files in ISO
    """
    
    def __init__(self):
        """Initialize the ISO mounter."""
        self._mounted_isos: Dict[str, ISOMountInfo] = {}
    
    def is_windows(self) -> bool:
        """Check if running on Windows."""
        return sys.platform.startswith("win")
    
    def mount_iso(
        self,
        iso_path: str,
        mount_dir: Optional[str] = None,
        drive_letter: Optional[str] = None,
    ) -> ISOMountInfo:
        """
        Mount an ISO file.
        
        Args:
            iso_path: Path to ISO file.
            mount_dir: Directory to mount to. If None, uses temp directory.
            drive_letter: Optional drive letter to mount as (Windows only).
            
        Returns:
            ISOMountInfo object.
        """
        if not self.is_windows():
            raise ISOMountError("ISO mounting is only supported on Windows")
        
        try:
            # Create mount directory if not provided
            if mount_dir is None:
                mount_dir = tempfile.mkdtemp(prefix="freent_iso_")
            else:
                os.makedirs(mount_dir, exist_ok=True)
            
            # Check if already mounted
            if iso_path in self._mounted_isos:
                raise ISOMountError(f"ISO {iso_path} already mounted")
            
            # Use PowerShell to mount ISO
            if drive_letter:
                # Mount with specific drive letter
                cmd = [
                    "powershell", "-Command",
                    f"Mount-DiskImage -ImagePath '{iso_path}' -PassThru | "
                    f"Get-Volume | Where-Object {{ $_.DriveLetter -eq '{drive_letter}' }} | "
                    f"Select-Object -ExpandProperty DriveLetter"
                ]
            else:
                # Mount and get drive letter
                cmd = [
                    "powershell", "-Command",
                    f"$mount = Mount-DiskImage -ImagePath '{iso_path}' -PassThru; "
                    f"$volume = Get-Volume | Where-Object {{ $_.Path -like '*{os.path.basename(iso_path)}*' }}; "
                    f"if ($volume) {{ $volume.DriveLetter }} else {{ (Get-Volume | Where-Object {{ $_.FileSystemLabel -eq '{os.path.splitext(os.path.basename(iso_path))[0]}' }}).DriveLetter }}"
                ]
            
            result = subprocess.run(
                cmd,
                capture_output=True,
                text=True,
                timeout=30,
            )
            
            if result.returncode != 0:
                raise ISOMountError(f"Failed to mount ISO: {result.stderr}")
            
            # Get the drive letter
            actual_drive = result.stdout.strip()
            
            # If we got a drive letter, use it
            if actual_drive and len(actual_drive) == 1 and actual_drive.isalpha():
                drive_path = f"{actual_drive}:\\"
                mount_info = ISOMountInfo(
                    iso_path=iso_path,
                    mount_dir=drive_path,
                    drive_letter=actual_drive.upper(),
                    virtual=False,
                )
            else:
                # Fallback: use virtual mount
                mount_info = ISOMountInfo(
                    iso_path=iso_path,
                    mount_dir=mount_dir,
                    virtual=True,
                )
            
            self._mounted_isos[iso_path] = mount_info
            
            return mount_info
            
        except Exception as e:
            # Clean up mount directory if created
            if mount_dir and os.path.exists(mount_dir):
                try:
                    shutil.rmtree(mount_dir)
                except Exception:
                    pass
            raise ISOMountError(f"Failed to mount ISO: {e}") from e
    
    def extract_iso(
        self,
        iso_path: str,
        extract_dir: Optional[str] = None,
        password: Optional[str] = None,
    ) -> str:
        """
        Extract an ISO file to a directory.
        
        Args:
            iso_path: Path to ISO file.
            extract_dir: Directory to extract to. If None, creates temp directory.
            password: Optional password for encrypted ISO.
            
        Returns:
            Path to extracted directory.
        """
        try:
            # Create extract directory if not provided
            if extract_dir is None:
                extract_dir = tempfile.mkdtemp(prefix="freent_iso_extract_")
            else:
                os.makedirs(extract_dir, exist_ok=True)
            
            # Try using 7-Zip first
            if self._extract_with_7zip(iso_path, extract_dir, password):
                return extract_dir
            
            # Try using PowerShell (Windows 10+)
            if self.is_windows() and self._extract_with_powershell(iso_path, extract_dir):
                return extract_dir
            
            # Try using python-lzma (if available)
            if self._extract_with_python(iso_path, extract_dir):
                return extract_dir
            
            raise ISOExtractError("No extraction method available")
            
        except Exception as e:
            # Clean up extract directory
            if extract_dir and os.path.exists(extract_dir):
                try:
                    shutil.rmtree(extract_dir)
                except Exception:
                    pass
            raise ISOExtractError(f"Failed to extract ISO: {e}") from e
    
    def _extract_with_7zip(
        self,
        iso_path: str,
        extract_dir: str,
        password: Optional[str] = None,
    ) -> bool:
        """Extract ISO using 7-Zip."""
        try:
            cmd = ["7z", "x", iso_path, f"-o{extract_dir}", "-y"]
            
            if password:
                cmd.extend([f"-p{password}"])
            
            result = subprocess.run(
                cmd,
                capture_output=True,
                text=True,
                timeout=300,
            )
            
            return result.returncode == 0
            
        except Exception:
            return False
    
    def _extract_with_powershell(
        self,
        iso_path: str,
        extract_dir: str,
    ) -> bool:
        """Extract ISO using PowerShell."""
        try:
            cmd = [
                "powershell", "-Command",
                f"$iso = Get-Content -Path '{iso_path}' -Encoding Byte -ReadCount 0; "
                f"[IO.File]::WriteAllBytes('{os.path.join(extract_dir, os.path.basename(iso_path))}', $iso); "
                f"Expand-Archive -Path '{os.path.join(extract_dir, os.path.basename(iso_path))}' -DestinationPath '{extract_dir}' -Force"
            ]
            
            result = subprocess.run(
                cmd,
                capture_output=True,
                text=True,
                timeout=300,
            )
            
            return result.returncode == 0
            
        except Exception:
            return False
    
    def _extract_with_python(
        self,
        iso_path: str,
        extract_dir: str,
    ) -> bool:
        """Extract ISO using Python libraries."""
        try:
            import pycdio
            
            # Use pycdio to read ISO
            iso = pycdio.Iso9660(iso_path)
            
            # Extract all files
            for children in iso.list_children():
                self._extract_iso_node(iso, children, extract_dir)
            
            return True
            
        except ImportError:
            return False
        except Exception:
            return False
    
    def _extract_iso_node(
        self,
        iso,
        node,
        base_dir: str,
    ) -> None:
        """Extract a node from ISO recursively."""
        path = os.path.join(base_dir, node.name)
        
        if node.is_dir():
            os.makedirs(path, exist_ok=True)
            for child in node.children:
                self._extract_iso_node(iso, child, path)
        else:
            os.makedirs(os.path.dirname(path), exist_ok=True)
            with open(path, "wb") as f:
                f.write(node.data)
    
    def unmount_iso(
        self,
        iso_path: str,
        force: bool = False,
    ) -> bool:
        """
        Unmount an ISO file.
        
        Args:
            iso_path: Path to ISO file.
            force: Whether to force unmount.
            
        Returns:
            True if successful, False otherwise.
        """
        if not self.is_windows():
            raise ISOMountError("ISO unmounting is only supported on Windows")
        
        try:
            # Check if mounted
            if iso_path not in self._mounted_isos:
                return True
            
            mount_info = self._mounted_isos[iso_path]
            
            # Use PowerShell to unmount
            if mount_info.drive_letter:
                cmd = [
                    "powershell", "-Command",
                    f"Dismount-DiskImage -ImagePath '{iso_path}' -Confirm:$false"
                ]
            else:
                # Virtual mount - just clean up directory
                if os.path.exists(mount_info.mount_dir):
                    shutil.rmtree(mount_info.mount_dir)
            
            result = subprocess.run(
                cmd,
                capture_output=True,
                text=True,
                timeout=30,
            )
            
            if result.returncode != 0 and not force:
                raise ISOMountError(f"Failed to unmount ISO: {result.stderr}")
            
            # Remove from mounted list
            del self._mounted_isos[iso_path]
            
            return True
            
        except Exception as e:
            raise ISOMountError(f"Failed to unmount ISO: {e}") from e
    
    def list_files(
        self,
        iso_path: str,
        extract_first: bool = False,
    ) -> List[ISOFileInfo]:
        """
        List files in an ISO.
        
        Args:
            iso_path: Path to ISO file.
            extract_first: If True, extract to temp dir first for accurate listing.
            
        Returns:
            List of ISOFileInfo objects.
        """
        try:
            if extract_first:
                # Extract to temp dir for accurate listing
                temp_dir = tempfile.mkdtemp(prefix="freent_iso_list_")
                try:
                    self.extract_iso(iso_path, temp_dir)
                    return self._list_extracted_files(temp_dir, "")
                finally:
                    shutil.rmtree(temp_dir, ignore_errors=True)
            else:
                # Try using 7-Zip to list
                return self._list_with_7zip(iso_path)
            
        except Exception as e:
            raise ISOError(f"Failed to list ISO files: {e}") from e
    
    def _list_extracted_files(
        self,
        base_dir: str,
        relative_path: str,
    ) -> List[ISOFileInfo]:
        """List files in extracted directory."""
        files = []
        
        for item in os.listdir(base_dir):
            item_path = os.path.join(base_dir, item)
            relative_item_path = os.path.join(relative_path, item) if relative_path else item
            
            if os.path.isdir(item_path):
                files.append(ISOFileInfo(
                    name=item,
                    path=relative_item_path,
                    size=0,
                    is_directory=True,
                ))
                files.extend(self._list_extracted_files(item_path, relative_item_path))
            else:
                files.append(ISOFileInfo(
                    name=item,
                    path=relative_item_path,
                    size=os.path.getsize(item_path),
                    is_directory=False,
                ))
        
        return files
    
    def _list_with_7zip(self, iso_path: str) -> List[ISOFileInfo]:
        """List files using 7-Zip."""
        try:
            result = subprocess.run(
                ["7z", "l", "-r", iso_path],
                capture_output=True,
                text=True,
                timeout=30,
            )
            
            if result.returncode != 0:
                return []
            
            files = []
            for line in result.stdout.split('\n')[2:]:  # Skip header
                parts = line.split()
                if len(parts) >= 6:
                    date, time, attr, size, compressed, name = parts[:6]
                    is_dir = attr.endswith('D')
                    
                    try:
                        file_size = int(size) if size.isdigit() else 0
                    except ValueError:
                        file_size = 0
                    
                    files.append(ISOFileInfo(
                        name=os.path.basename(name),
                        path=name,
                        size=file_size,
                        is_directory=is_dir,
                    ))
            
            return files
            
        except Exception:
            return []
    
    def find_wim_files(
        self,
        iso_path: str,
        extract_first: bool = True,
    ) -> List[str]:
        """
        Find WIM files in an ISO.
        
        Args:
            iso_path: Path to ISO file.
            extract_first: If True, extract to temp dir first.
            
        Returns:
            List of paths to WIM files.
        """
        wim_files = []
        
        try:
            files = self.list_files(iso_path, extract_first=extract_first)
            
            for file_info in files:
                if file_info.name.lower().endswith(".wim") or \
                   file_info.name.lower().endswith(".esd"):
                    wim_files.append(file_info.path)
            
            return wim_files
            
        except Exception as e:
            raise ISOError(f"Failed to find WIM files: {e}") from e
    
    def get_wim_path_from_iso(
        self,
        iso_path: str,
        edition: Optional[str] = None,
    ) -> Optional[str]:
        """
        Get the path to the main WIM file in an ISO.
        
        Args:
            iso_path: Path to ISO file.
            edition: Optional edition to look for (e.g., "Enterprise").
            
        Returns:
            Path to WIM file, or None if not found.
        """
        try:
            # Extract ISO to temp directory
            temp_dir = tempfile.mkdtemp(prefix="freent_iso_wim_")
            
            try:
                self.extract_iso(iso_path, temp_dir)
                
                # Look for WIM files
                for root, dirs, files in os.walk(temp_dir):
                    for file in files:
                        if file.lower().endswith(".wim") or file.lower().endswith(".esd"):
                            # Check if this is the right edition
                            if edition:
                                # Check if the directory contains the edition name
                                if edition.lower() in root.lower():
                                    return os.path.join(root, file)
                            else:
                                # Return the first WIM file found
                                return os.path.join(root, file)
                
                # If no specific edition found, return the first WIM
                for root, dirs, files in os.walk(temp_dir):
                    for file in files:
                        if file.lower().endswith(".wim") or file.lower().endswith(".esd"):
                            return os.path.join(root, file)
                
                return None
                
            finally:
                shutil.rmtree(temp_dir, ignore_errors=True)
            
        except Exception as e:
            raise ISOError(f"Failed to get WIM path from ISO: {e}") from e
    
    def cleanup(self) -> None:
        """Clean up all mounted ISOs."""
        for iso_path, mount_info in list(self._mounted_isos.items()):
            try:
                self.unmount_iso(iso_path)
            except Exception as e:
                print(f"Warning: Failed to clean up {iso_path}: {e}")
        
        self._mounted_isos.clear()
    
    def get_mounted_isos(self) -> List[ISOMountInfo]:
        """Get list of currently mounted ISOs."""
        return list(self._mounted_isos.values())
    
    def is_mounted(self, iso_path: str) -> bool:
        """Check if an ISO is mounted."""
        return iso_path in self._mounted_isos


# Global instance
iso_mounter = ISOMounter()


if __name__ == "__main__":
    # Example usage
    print("ISO Mounter Example")
    print("==================")
    
    mounter = ISOMounter()
    
    # Example ISO path (change this to a real ISO path)
    iso_path = "C:\\Windows\System32\install.iso"
    
    if os.path.exists(iso_path):
        print(f"\nISO File: {iso_path}")
        
        # List files
        try:
            files = mounter.list_files(iso_path, extract_first=False)
            print(f"Found {len(files)} files in ISO")
            
            # Find WIM files
            wim_files = mounter.find_wim_files(iso_path, extract_first=False)
            print(f"Found {len(wim_files)} WIM/ESD files:")
            for wim_file in wim_files:
                print(f"  - {wim_file}")
        except Exception as e:
            print(f"Error: {e}")
    else:
        print(f"ISO file not found: {iso_path}")
