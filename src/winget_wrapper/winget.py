# FreeNT Winget Wrapper - Main Implementation
# Copyright (c) 2026, Panoc95
# BSD 3-Clause License

"""
Winget wrapper implementation for FreeNT.
This module provides a Python interface to Microsoft's winget package manager.
"""

import asyncio
import json
import subprocess
import sys
from dataclasses import dataclass
from enum import Enum
from typing import Optional, List, Dict, Any


class WingetError(Exception):
    """Base exception for winget wrapper errors."""
    pass


class WingetNotFoundError(WingetError):
    """Raised when winget is not found in the system."""
    pass


class WingetCommandError(WingetError):
    """Raised when a winget command fails."""
    def __init__(self, command: str, returncode: int, stderr: str = ""):
        self.command = command
        self.returncode = returncode
        self.stderr = stderr
        super().__init__(f"Command '{command}' failed with code {returncode}: {stderr}")


class PackageSource(str, Enum):
    """Package source types for winget."""
    WINGET = "winget"
    MSSTORE = "msstore"
    ALL = "all"


@dataclass
class PackageInfo:
    """Information about a package."""
    id: str
    name: str
    version: str
    source: str
    publisher: str
    description: str = ""
    homepage: str = ""
    license: str = ""
    tags: List[str] = None
    
    def __post_init__(self):
        if self.tags is None:
            self.tags = []
    
    def to_dict(self) -> Dict[str, Any]:
        """Convert to dictionary."""
        return {
            "id": self.id,
            "name": self.name,
            "version": self.version,
            "source": self.source,
            "publisher": self.publisher,
            "description": self.description,
            "homepage": self.homepage,
            "license": self.license,
            "tags": self.tags,
        }


class WingetWrapper:
    """
    Wrapper for Microsoft's winget package manager.
    
    This class provides a Python interface to winget, allowing for
    package search, installation, removal, and management.
    """

    WINGET_EXECUTABLE = "winget.exe"

    def __init__(self, winget_path: Optional[str] = None):
        """
        Initialize the winget wrapper.
        
        Args:
            winget_path: Optional path to winget executable. If None, uses default.
        """
        self.winget_path = winget_path or self.WINGET_EXECUTABLE
        self._check_winget_available()

    def _check_winget_available(self) -> None:
        """Check if winget is available on the system."""
        try:
            result = subprocess.run(
                [self.winget_path, "--version"],
                capture_output=True,
                text=True,
                check=True,
            )
            self.version = result.stdout.strip()
        except (subprocess.CalledProcessError, FileNotFoundError) as e:
            raise WingetNotFoundError(
                f"Winget not found at {self.winget_path}. "
                "Ensure winget is installed and in your PATH."
            ) from e

    def _run_command(
        self,
        args: List[str],
        check: bool = True,
        capture_output: bool = True,
    ) -> subprocess.CompletedProcess[str]:
        """
        Run a winget command.
        
        Args:
            args: List of arguments to pass to winget.
            check: Whether to raise an exception on non-zero exit code.
            capture_output: Whether to capture stdout and stderr.
            
        Returns:
            CompletedProcess with command results.
        """
        command = [self.winget_path] + args
        
        try:
            result = subprocess.run(
                command,
                capture_output=capture_output,
                text=True,
                check=check,
            )
            return result
        except subprocess.CalledProcessError as e:
            raise WingetCommandError(
                command=" ".join(command),
                returncode=e.returncode,
                stderr=e.stderr,
            ) from e

    async def _run_command_async(
        self,
        args: List[str],
        check: bool = True,
        capture_output: bool = True,
    ) -> subprocess.CompletedProcess[str]:
        """
        Run a winget command asynchronously.
        
        Args:
            args: List of arguments to pass to winget.
            check: Whether to raise an exception on non-zero exit code.
            capture_output: Whether to capture stdout and stderr.
            
        Returns:
            CompletedProcess with command results.
        """
        command = [self.winget_path] + args
        
        try:
            result = await asyncio.create_subprocess_exec(
                *command,
                stdout=asyncio.subprocess.PIPE if capture_output else None,
                stderr=asyncio.subprocess.PIPE if capture_output else None,
            )
            stdout, stderr = await result.communicate()
            
            if check and result.returncode != 0:
                raise WingetCommandError(
                    command=" ".join(command),
                    returncode=result.returncode,
                    stderr=stderr.decode() if stderr else "",
                )
            
            return subprocess.CompletedProcess(
                args=command,
                returncode=result.returncode,
                stdout=stdout.decode() if stdout else "",
                stderr=stderr.decode() if stderr else "",
            )
        except Exception as e:
            raise WingetCommandError(
                command=" ".join(command),
                returncode=-1,
                stderr=str(e),
            ) from e

    def search(
        self,
        query: str,
        source: PackageSource = PackageSource.ALL,
        exact: bool = False,
        limit: Optional[int] = None,
    ) -> List[PackageInfo]:
        """
        Search for packages.
        
        Args:
            query: Search query.
            source: Package source to search (winget, msstore, or all).
            exact: Whether to perform exact match search.
            limit: Maximum number of results to return.
            
        Returns:
            List of PackageInfo objects matching the search.
        """
        args = ["search", query]
        
        if source != PackageSource.ALL:
            args.extend(["--source", source.value])
        
        if exact:
            args.append("--exact")
        
        if limit:
            args.extend(["--count", str(limit)])
        
        args.extend(["--accept-source-agreements", "--accept-package-agreements"])
        args.append("--json")
        
        result = self._run_command(args)
        
        try:
            data = json.loads(result.stdout)
            packages = []
            
            for item in data.get("Packages", []):
                pkg = PackageInfo(
                    id=item.get("PackageIdentifier", ""),
                    name=item.get("Name", ""),
                    version=item.get("Version", ""),
                    source=item.get("Source", ""),
                    publisher=item.get("Publisher", ""),
                    description=item.get("Description", ""),
                    homepage=item.get("Homepage", ""),
                    license=item.get("License", ""),
                    tags=item.get("Tags", []),
                )
                packages.append(pkg)
            
            return packages
        except json.JSONDecodeError as e:
            raise WingetError(f"Failed to parse winget output: {e}") from e

    async def search_async(
        self,
        query: str,
        source: PackageSource = PackageSource.ALL,
        exact: bool = False,
        limit: Optional[int] = None,
    ) -> List[PackageInfo]:
        """
        Search for packages asynchronously.
        
        Args:
            query: Search query.
            source: Package source to search (winget, msstore, or all).
            exact: Whether to perform exact match search.
            limit: Maximum number of results to return.
            
        Returns:
            List of PackageInfo objects matching the search.
        """
        args = ["search", query]
        
        if source != PackageSource.ALL:
            args.extend(["--source", source.value])
        
        if exact:
            args.append("--exact")
        
        if limit:
            args.extend(["--count", str(limit)])
        
        args.extend(["--accept-source-agreements", "--accept-package-agreements"])
        args.append("--json")
        
        result = await self._run_command_async(args)
        
        try:
            data = json.loads(result.stdout)
            packages = []
            
            for item in data.get("Packages", []):
                pkg = PackageInfo(
                    id=item.get("PackageIdentifier", ""),
                    name=item.get("Name", ""),
                    version=item.get("Version", ""),
                    source=item.get("Source", ""),
                    publisher=item.get("Publisher", ""),
                    description=item.get("Description", ""),
                    homepage=item.get("Homepage", ""),
                    license=item.get("License", ""),
                    tags=item.get("Tags", []),
                )
                packages.append(pkg)
            
            return packages
        except json.JSONDecodeError as e:
            raise WingetError(f"Failed to parse winget output: {e}") from e

    def install(
        self,
        package_id: str,
        version: Optional[str] = None,
        source: Optional[PackageSource] = None,
        silent: bool = False,
        accept_agreements: bool = True,
    ) -> subprocess.CompletedProcess[str]:
        """
        Install a package.
        
        Args:
            package_id: Package identifier to install.
            version: Optional version to install.
            source: Optional package source.
            silent: Whether to install silently.
            accept_agreements: Whether to automatically accept agreements.
            
        Returns:
            CompletedProcess with installation results.
        """
        args = ["install", package_id]
        
        if version:
            args.extend(["--version", version])
        
        if source:
            args.extend(["--source", source.value])
        
        if silent:
            args.append("--silent")
        
        if accept_agreements:
            args.extend(["--accept-source-agreements", "--accept-package-agreements"])
        
        return self._run_command(args)

    async def install_async(
        self,
        package_id: str,
        version: Optional[str] = None,
        source: Optional[PackageSource] = None,
        silent: bool = False,
        accept_agreements: bool = True,
    ) -> subprocess.CompletedProcess[str]:
        """
        Install a package asynchronously.
        
        Args:
            package_id: Package identifier to install.
            version: Optional version to install.
            source: Optional package source.
            silent: Whether to install silently.
            accept_agreements: Whether to automatically accept agreements.
            
        Returns:
            CompletedProcess with installation results.
        """
        args = ["install", package_id]
        
        if version:
            args.extend(["--version", version])
        
        if source:
            args.extend(["--source", source.value])
        
        if silent:
            args.append("--silent")
        
        if accept_agreements:
            args.extend(["--accept-source-agreements", "--accept-package-agreements"])
        
        return await self._run_command_async(args)

    def uninstall(
        self,
        package_id: str,
        version: Optional[str] = None,
        source: Optional[PackageSource] = None,
        silent: bool = False,
    ) -> subprocess.CompletedProcess[str]:
        """
        Uninstall a package.
        
        Args:
            package_id: Package identifier to uninstall.
            version: Optional version to uninstall.
            source: Optional package source.
            silent: Whether to uninstall silently.
            
        Returns:
            CompletedProcess with uninstallation results.
        """
        args = ["uninstall", package_id]
        
        if version:
            args.extend(["--version", version])
        
        if source:
            args.extend(["--source", source.value])
        
        if silent:
            args.append("--silent")
        
        return self._run_command(args)

    async def uninstall_async(
        self,
        package_id: str,
        version: Optional[str] = None,
        source: Optional[PackageSource] = None,
        silent: bool = False,
    ) -> subprocess.CompletedProcess[str]:
        """
        Uninstall a package asynchronously.
        
        Args:
            package_id: Package identifier to uninstall.
            version: Optional version to uninstall.
            source: Optional package source.
            silent: Whether to uninstall silently.
            
        Returns:
            CompletedProcess with uninstallation results.
        """
        args = ["uninstall", package_id]
        
        if version:
            args.extend(["--version", version])
        
        if source:
            args.extend(["--source", source.value])
        
        if silent:
            args.append("--silent")
        
        return await self._run_command_async(args)

    def list_installed(self) -> List[PackageInfo]:
        """
        List all installed packages.
        
        Returns:
            List of PackageInfo objects for installed packages.
        """
        args = ["list", "--json"]
        result = self._run_command(args)
        
        try:
            data = json.loads(result.stdout)
            packages = []
            
            for item in data.get("Packages", []):
                pkg = PackageInfo(
                    id=item.get("PackageIdentifier", ""),
                    name=item.get("Name", ""),
                    version=item.get("Version", ""),
                    source=item.get("Source", ""),
                    publisher=item.get("Publisher", ""),
                )
                packages.append(pkg)
            
            return packages
        except json.JSONDecodeError as e:
            raise WingetError(f"Failed to parse winget output: {e}") from e

    async def list_installed_async(self) -> List[PackageInfo]:
        """
        List all installed packages asynchronously.
        
        Returns:
            List of PackageInfo objects for installed packages.
        """
        args = ["list", "--json"]
        result = await self._run_command_async(args)
        
        try:
            data = json.loads(result.stdout)
            packages = []
            
            for item in data.get("Packages", []):
                pkg = PackageInfo(
                    id=item.get("PackageIdentifier", ""),
                    name=item.get("Name", ""),
                    version=item.get("Version", ""),
                    source=item.get("Source", ""),
                    publisher=item.get("Publisher", ""),
                )
                packages.append(pkg)
            
            return packages
        except json.JSONDecodeError as e:
            raise WingetError(f"Failed to parse winget output: {e}") from e

    def upgrade(self, package_id: Optional[str] = None) -> subprocess.CompletedProcess[str]:
        """
        Upgrade packages.
        
        Args:
            package_id: Optional package identifier to upgrade. If None, upgrades all.
            
        Returns:
            CompletedProcess with upgrade results.
        """
        args = ["upgrade"]
        
        if package_id:
            args.append(package_id)
        else:
            args.append("--all")
        
        args.extend(["--accept-source-agreements", "--accept-package-agreements"])
        
        return self._run_command(args)

    async def upgrade_async(self, package_id: Optional[str] = None) -> subprocess.CompletedProcess[str]:
        """
        Upgrade packages asynchronously.
        
        Args:
            package_id: Optional package identifier to upgrade. If None, upgrades all.
            
        Returns:
            CompletedProcess with upgrade results.
        """
        args = ["upgrade"]
        
        if package_id:
            args.append(package_id)
        else:
            args.append("--all")
        
        args.extend(["--accept-source-agreements", "--accept-package-agreements"])
        
        return await self._run_command_async(args)

    def get_version(self) -> str:
        """
        Get the version of winget.
        
        Returns:
            Version string of winget.
        """
        return self.version

    def is_available(self) -> bool:
        """
        Check if winget is available.
        
        Returns:
            True if winget is available, False otherwise.
        """
        try:
            self._check_winget_available()
            return True
        except WingetNotFoundError:
            return False


# Global instance for convenience
winget = WingetWrapper()


if __name__ == "__main__":
    # Example usage
    try:
        wrapper = WingetWrapper()
        print(f"Winget version: {wrapper.get_version()}")
        
        # Search for packages
        print("\nSearching for 'python'...")
        results = wrapper.search("python", limit=5)
        for pkg in results:
            print(f"  {pkg.id} - {pkg.name} ({pkg.version})")
        
        # List installed packages
        print("\nInstalled packages:")
        installed = wrapper.list_installed()
        for pkg in installed[:5]:  # Show first 5
            print(f"  {pkg.id} - {pkg.name} ({pkg.version})")
        
    except WingetError as e:
        print(f"Error: {e}")
        sys.exit(1)
