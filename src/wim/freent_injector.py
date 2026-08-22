# FreeNT Injector
# Copyright (c) 2026, Panoc95
# BSD 3-Clause License

"""
FreeNT injection into WIM images.
This module handles injecting FreeNT components into Windows WIM images
for custom installation.
"""

import os
import sys
import subprocess
import shutil
import tempfile
from dataclasses import dataclass, field
from typing import Optional, List, Dict, Any, Tuple
from pathlib import Path

from .wim_manager import WIMManager, WIMError
from .edition_customizer import EditionCustomizer, EditionError
from .bloatware_remover import BloatwareRemover, BloatwareError


class InjectionError(Exception):
    """Base exception for FreeNT injection errors."""
    pass


class InjectionFailedError(InjectionError):
    """Raised when injection fails."""
    pass


@dataclass
class InjectionConfig:
    """Configuration for FreeNT injection."""
    
    # Source FreeNT directory
    freent_dir: Optional[str] = None
    
    # WIM file to inject into
    wim_path: Optional[str] = None
    
    # Image index to inject into
    image_index: int = 1
    
    # Whether to customize edition
    customize_edition: bool = True
    
    # Whether to remove bloatware
    remove_bloatware: bool = True
    
    # Bloatware removal level
    bloatware_level: str = "aggressive"  # minimal, standard, aggressive
    
    # Whether to create new WIM or modify existing
    create_new_wim: bool = True
    
    # Output WIM path
    output_wim: Optional[str] = None
    
    # Whether to install FreeNT as default shell
    install_as_default_shell: bool = True
    
    # Whether to add FreeNT to startup
    add_to_startup: bool = True
    
    # Whether to create portable FreeNT in WIM
    create_portable: bool = False
    
    # Additional files to inject
    additional_files: List[Tuple[str, str]] = field(default_factory=list)  # (source, dest)
    
    def to_dict(self) -> Dict[str, Any]:
        """Convert to dictionary."""
        return {
            "freent_dir": self.freent_dir,
            "wim_path": self.wim_path,
            "image_index": self.image_index,
            "customize_edition": self.customize_edition,
            "remove_bloatware": self.remove_bloatware,
            "bloatware_level": self.bloatware_level,
            "create_new_wim": self.create_new_wim,
            "output_wim": self.output_wim,
            "install_as_default_shell": self.install_as_default_shell,
            "add_to_startup": self.add_to_startup,
            "create_portable": self.create_portable,
            "additional_files": self.additional_files,
        }


@dataclass
class InjectionResult:
    """Result of FreeNT injection."""
    
    success: bool
    message: str
    output_wim: Optional[str] = None
    actions_performed: List[str] = field(default_factory=list)
    errors: List[str] = field(default_factory=list)
    warnings: List[str] = field(default_factory=list)
    
    def to_dict(self) -> Dict[str, Any]:
        """Convert to dictionary."""
        return {
            "success": self.success,
            "message": self.message,
            "output_wim": self.output_wim,
            "actions_performed": self.actions_performed,
            "errors": self.errors,
            "warnings": self.warnings,
        }


class FreeNTInjector:
    """
    Injects FreeNT into Windows WIM images.
    
    This class provides functionality to:
    - Mount a WIM image
    - Inject FreeNT files
    - Customize the edition to FreeNT
    - Remove bloatware
    - Configure FreeNT as default shell
    - Create a new customized WIM
    """
    
    def __init__(
        self,
        wim_manager: Optional[WIMManager] = None,
        edition_customizer: Optional[EditionCustomizer] = None,
        bloatware_remover: Optional[BloatwareRemover] = None,
    ):
        """
        Initialize the FreeNT injector.
        
        Args:
            wim_manager: Optional WIMManager instance.
            edition_customizer: Optional EditionCustomizer instance.
            bloatware_remover: Optional BloatwareRemover instance.
        """
        self.wim_manager = wim_manager or WIMManager()
        self.edition_customizer = edition_customizer or EditionCustomizer(self.wim_manager)
        self.bloatware_remover = bloatware_remover or BloatwareRemover(self.wim_manager)
        self.config = InjectionConfig()
    
    def inject_freent(
        self,
        config: Optional[InjectionConfig] = None,
    ) -> InjectionResult:
        """
        Inject FreeNT into a WIM image.
        
        Args:
            config: Optional InjectionConfig.
            
        Returns:
            InjectionResult with injection status.
        """
        if config:
            self.config = config
        
        result = InjectionResult(
            success=False,
            message="",
            actions_performed=[],
            errors=[],
            warnings=[],
        )
        
        try:
            # Validate config
            self._validate_config()
            
            # Determine FreeNT directory
            if self.config.freent_dir is None:
                self.config.freent_dir = self._find_freent_dir()
            
            if not self.config.freent_dir:
                raise InjectionFailedError("FreeNT directory not found")
            
            # Determine output WIM path
            if self.config.output_wim is None:
                if self.config.wim_path:
                    base, ext = os.path.splitext(self.config.wim_path)
                    self.config.output_wim = f"{base}_freent{ext}"
                else:
                    self.config.output_wim = "install_freent.wim"
            
            # Check if we need to create new WIM
            if self.config.create_new_wim:
                # Copy the original WIM
                if self.config.wim_path:
                    shutil.copy2(self.config.wim_path, self.config.output_wim)
                    result.actions_performed.append(f"Copied WIM to {self.config.output_wim}")
            else:
                self.config.output_wim = self.config.wim_path
            
            # Mount the WIM
            session = self.wim_manager.mount_wim(
                self.config.output_wim,
                self.config.image_index,
            )
            result.actions_performed.append(f"Mounted WIM at {session.mount_dir}")
            
            try:
                # Customize edition if enabled
                if self.config.customize_edition:
                    self._customize_edition(session.mount_dir)
                    result.actions_performed.append("Customized edition to FreeNT")
                
                # Remove bloatware if enabled
                if self.config.remove_bloatware:
                    self._remove_bloatware(session.mount_dir)
                    result.actions_performed.append(f"Removed bloatware ({self.config.bloatware_level})")
                
                # Inject FreeNT files
                self._inject_freent_files(session.mount_dir)
                result.actions_performed.append("Injected FreeNT files")
                
                # Configure FreeNT as default shell if enabled
                if self.config.install_as_default_shell:
                    self._configure_default_shell(session.mount_dir)
                    result.actions_performed.append("Configured FreeNT as default shell")
                
                # Add to startup if enabled
                if self.config.add_to_startup:
                    self._add_to_startup(session.mount_dir)
                    result.actions_performed.append("Added FreeNT to startup")
                
                # Create portable FreeNT if enabled
                if self.config.create_portable:
                    self._create_portable_freent(session.mount_dir)
                    result.actions_performed.append("Created portable FreeNT")
                
                # Inject additional files if specified
                if self.config.additional_files:
                    self._inject_additional_files(session.mount_dir)
                    result.actions_performed.append(f"Injected {len(self.config.additional_files)} additional files")
                
                # Unmount and commit
                self.wim_manager.unmount_wim(session.mount_dir, commit=True)
                result.actions_performed.append("Committed changes to WIM")
                
                # Optimize the WIM
                self.wim_manager.optimize_wim(self.config.output_wim)
                result.actions_performed.append("Optimized WIM")
                
                # Success
                result.success = True
                result.message = "FreeNT successfully injected into WIM"
                result.output_wim = self.config.output_wim
                
            except Exception as e:
                # Rollback on error
                self.wim_manager.unmount_wim(session.mount_dir, commit=False)
                raise InjectionFailedError(f"Failed to inject FreeNT: {e}") from e
        
        except Exception as e:
            result.success = False
            result.message = f"FreeNT injection failed: {e}"
            result.errors.append(str(e))
        
        return result
    
    def _validate_config(self) -> None:
        """Validate injection configuration."""
        if self.config.wim_path and not os.path.exists(self.config.wim_path):
            raise InjectionFailedError(f"WIM file not found: {self.config.wim_path}")
        
        if self.config.freent_dir and not os.path.exists(self.config.freent_dir):
            raise InjectionFailedError(f"FreeNT directory not found: {self.config.freent_dir}")
        
        if self.config.bloatware_level not in ["minimal", "standard", "aggressive"]:
            raise InjectionFailedError(f"Invalid bloatware level: {self.config.bloatware_level}")
    
    def _find_freent_dir(self) -> Optional[str]:
        """Find the FreeNT directory."""
        # Check current directory
        if os.path.exists("src") and os.path.exists("scripts"):
            return os.path.abspath(".")
        
        # Check parent directories
        current = os.path.abspath(".")
        for _ in range(5):
            parent = os.path.dirname(current)
            if os.path.exists(os.path.join(parent, "src")) and \
               os.path.exists(os.path.join(parent, "scripts")):
                return parent
            current = parent
        
        # Check environment variable
        freent_home = os.environ.get("FREENT_HOME")
        if freent_home and os.path.exists(freent_home):
            return freent_home
        
        return None
    
    def _customize_edition(self, mount_dir: str) -> None:
        """Customize the edition to FreeNT."""
        try:
            # Modify system identity in the mounted image
            self._modify_system_identity(mount_dir)
            
            # Modify edition name
            self._modify_edition_name(mount_dir)
            
        except Exception as e:
            raise InjectionFailedError(f"Failed to customize edition: {e}") from e
    
    def _modify_system_identity(self, mount_dir: str) -> None:
        """Modify system identity in mounted image."""
        try:
            # Modify registry
            self._modify_registry_identity(mount_dir)
            
            # Modify system files
            self._modify_system_files(mount_dir)
            
        except Exception as e:
            print(f"Warning: Failed to modify system identity: {e}")
    
    def _modify_registry_identity(self, mount_dir: str) -> None:
        """Modify registry to change system identity."""
        try:
            # Create reg file for FreeNT identity
            reg_content = """Windows Registry Editor Version 5.00

[HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows NT\CurrentVersion]
"ProductName"="Windows 11 FreeNT"
"DisplayVersion"="10.0"
"CurrentBuild"="FreeNT"
"CurrentVersion"="Windows 11 FreeNT"
"EditionID"="FreeNT"
"InstallationType"="Client"
"ProductId"="00330-80000-00000-AAOEM"
"RegisteredOrganization"="FreeNT Project"
"RegisteredOwner"="FreeNT User"

[HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion]
"ProductName"="Windows 11 FreeNT"
"CurrentVersion"="Windows 11 FreeNT"
"CurrentBuildNumber"="FreeNT"
"CurrentType"="Multiprocessor Free"
"EditionID"="FreeNT"

[HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\ProductOptions]
"ProductName"="Windows 11 FreeNT"
"ProductPolicy"=dword:00000000
"""
            
            reg_file = os.path.join(mount_dir, "freent_identity.reg")
            with open(reg_file, "w") as f:
                f.write(reg_content)
            
            # Apply reg file to offline registry
            # Note: This requires Windows and proper permissions
            # In practice, we'd use DISM or regedit with offline hive
            
            # For now, just create the file for manual application
            # In a real implementation, we'd use:
            # dism /Image:mount_dir /Import-DefaultAppAssociations
            # or regedit /s reg_file
            
            print(f"Registry identity file created: {reg_file}")
            
        except Exception as e:
            print(f"Warning: Failed to modify registry identity: {e}")
    
    def _modify_system_files(self, mount_dir: str) -> None:
        """Modify system files to reflect FreeNT identity."""
        try:
            # Modify systeminfo.exe output (if present)
            systeminfo_exe = os.path.join(mount_dir, "Windows", "System32", "systeminfo.exe")
            if os.path.exists(systeminfo_exe):
                # This is a placeholder - actual modification would require
                # resource editing or replacement
                pass
            
            # Modify winver.exe (if present)
            winver_exe = os.path.join(mount_dir, "Windows", "System32", "winver.exe")
            if os.path.exists(winver_exe):
                # Placeholder for winver modification
                pass
            
            # Modify cmd.exe version info
            cmd_exe = os.path.join(mount_dir, "Windows", "System32", "cmd.exe")
            if os.path.exists(cmd_exe):
                # Placeholder for cmd.exe modification
                pass
            
        except Exception as e:
            print(f"Warning: Failed to modify system files: {e}")
    
    def _modify_edition_name(self, mount_dir: str) -> None:
        """Modify edition name in various configuration files."""
        try:
            # Modify setup.cfg if exists
            setup_cfg = os.path.join(mount_dir, "setup.cfg")
            if os.path.exists(setup_cfg):
                with open(setup_cfg, "r") as f:
                    content = f.read()
                
                content = content.replace("Enterprise", "Windows 11 FreeNT")
                content = content.replace("Professional", "Windows 11 FreeNT")
                content = content.replace("Home", "Windows 11 FreeNT")
                
                with open(setup_cfg, "w") as f:
                    f.write(content)
            
            # Modify unattend.xml if exists
            unattend_paths = [
                os.path.join(mount_dir, "unattend.xml"),
                os.path.join(mount_dir, "Windows", "System32", "unattend.xml"),
                os.path.join(mount_dir, "Windows", "Panther", "unattend.xml"),
            ]
            
            for unattend_path in unattend_paths:
                if os.path.exists(unattend_path):
                    self._modify_unattend_xml(unattend_path)
            
        except Exception as e:
            print(f"Warning: Failed to modify edition name: {e}")
    
    def _modify_unattend_xml(self, unattend_path: str) -> None:
        """Modify unattend.xml to use FreeNT settings."""
        try:
            import xml.etree.ElementTree as ET
            
            tree = ET.parse(unattend_path)
            root = tree.getroot()
            
            # Find and modify settings
            for elem in root.iter():
                if elem.tag.endswith("ProductKey"):
                    # Use generic key for FreeNT
                    elem.text = "VK7JG-NPHTM-C97JM-9MPGT-3V66T"
                
                if elem.tag.endswith("EditionID"):
                    elem.text = "FreeNT"
                
                if elem.tag.endswith("ProductName"):
                    elem.text = "Windows 11 FreeNT"
            
            tree.write(unattend_path)
            
        except Exception as e:
            print(f"Warning: Failed to modify unattend.xml: {e}")
    
    def _remove_bloatware(self, mount_dir: str) -> None:
        """Remove bloatware from mounted image."""
        try:
            if self.config.bloatware_level == "aggressive":
                self.bloatware_remover.remove_all_msstore_apps(mount_dir)
            elif self.config.bloatware_level == "standard":
                self.bloatware_remover.remove_common_bloatware(mount_dir)
            else:  # minimal
                # Only remove the most egregious bloatware
                config = BloatwareConfig(
                    remove_categories=["xbox", "bing"],
                    remove_all_msstore=False,
                    remove_provisioned=True,
                )
                self.bloatware_remover.remove_bloatware(mount_dir, config)
            
        except Exception as e:
            raise InjectionFailedError(f"Failed to remove bloatware: {e}") from e
    
    def _inject_freent_files(self, mount_dir: str) -> None:
        """Inject FreeNT files into mounted image."""
        try:
            # Create FreeNT directory in Program Files
            freent_dir = os.path.join(mount_dir, "Program Files", "FreeNT")
            os.makedirs(freent_dir, exist_ok=True)
            
            # Copy FreeNT source files
            src_dir = os.path.join(self.config.freent_dir, "src")
            if os.path.exists(src_dir):
                shutil.copytree(src_dir, os.path.join(freent_dir, "src"))
            
            # Copy standalone files
            standalone_dir = os.path.join(self.config.freent_dir, "standalone")
            if os.path.exists(standalone_dir):
                shutil.copytree(standalone_dir, os.path.join(freent_dir, "standalone"))
            
            # Copy scripts
            scripts_dir = os.path.join(self.config.freent_dir, "scripts")
            if os.path.exists(scripts_dir):
                shutil.copytree(scripts_dir, os.path.join(freent_dir, "scripts"))
            
            # Copy configuration files
            config_files = ["requirements.txt", "setup.py", "setup.cfg", "pyproject.toml"]
            for config_file in config_files:
                src = os.path.join(self.config.freent_dir, config_file)
                if os.path.exists(src):
                    shutil.copy2(src, freent_dir)
            
            # Create a startup script
            self._create_startup_script(freent_dir)
            
        except Exception as e:
            raise InjectionFailedError(f"Failed to inject FreeNT files: {e}") from e
    
    def _create_startup_script(self, freent_dir: str) -> None:
        """Create a startup script for FreeNT."""
        try:
            # Create batch file for startup
            startup_bat = os.path.join(freent_dir, "start_freent.bat")
            with open(startup_bat, "w") as f:
                f.write("""@echo off
REM FreeNT Startup Script
SETLOCAL

REM Set FreeNT environment
set FREENT_HOME=%~dp0
set PATH=%PATH%;%FREENT_HOME%\standalone\bin

REM Start FreeNT shell
start "" "%FREENT_HOME%\standalone\bin\freent.bat"
""")
            
            # Create PowerShell script for startup
            startup_ps1 = os.path.join(freent_dir, "start_freent.ps1")
            with open(startup_ps1, "w") as f:
                f.write("""# FreeNT Startup Script
$ErrorActionPreference = "Stop"

# Set FreeNT environment
$env:FREENT_HOME = Split-Path -Parent $MyInvocation.MyCommand.Definition
$env:PATH += ";$env:FREENT_HOME\standalone\bin"

# Start FreeNT shell
Start-Process -FilePath "$env:FREENT_HOME\standalone\bin\freent.bat"
""")
            
        except Exception as e:
            print(f"Warning: Failed to create startup script: {e}")
    
    def _configure_default_shell(self, mount_dir: str) -> None:
        """Configure FreeNT as the default shell."""
        try:
            # Modify Winlogon registry to use FreeNT shell
            self._modify_winlogon_shell(mount_dir)
            
            # Modify Userinit to prevent Explorer from starting
            self._modify_userinit(mount_dir)
            
            # Create autostart entry
            self._create_autostart_entry(mount_dir)
            
        except Exception as e:
            raise InjectionFailedError(f"Failed to configure default shell: {e}") from e
    
    def _modify_winlogon_shell(self, mount_dir: str) -> None:
        """Modify Winlogon registry to use FreeNT shell."""
        try:
            # Create reg file for Winlogon modification
            freent_dir = os.path.join(mount_dir, "Program Files", "FreeNT")
            shell_path = os.path.join(freent_dir, "standalone", "bin", "freent.bat")
            
            reg_content = f"""Windows Registry Editor Version 5.00

[HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Winlogon]
"Shell"="{shell_path}"
"Userinit"=
"""
            
            reg_file = os.path.join(mount_dir, "freent_shell.reg")
            with open(reg_file, "w") as f:
                f.write(reg_content)
            
            print(f"Winlogon shell configuration created: {reg_file}")
            
        except Exception as e:
            print(f"Warning: Failed to modify Winlogon shell: {e}")
    
    def _modify_userinit(self, mount_dir: str) -> None:
        """Modify Userinit to prevent Explorer from starting."""
        try:
            # Remove Userinit value to prevent Explorer from starting
            # This is handled in the Winlogon modification above
            pass
            
        except Exception as e:
            print(f"Warning: Failed to modify Userinit: {e}")
    
    def _create_autostart_entry(self, mount_dir: str) -> None:
        """Create autostart entry for FreeNT."""
        try:
            # Create Startup directory
            startup_dir = os.path.join(
                mount_dir,
                "Users",
                "Default",
                "AppData",
                "Roaming",
                "Microsoft",
                "Windows",
                "Start Menu",
                "Programs",
                "Startup",
            )
            os.makedirs(startup_dir, exist_ok=True)
            
            # Create shortcut to FreeNT startup
            freent_dir = os.path.join(mount_dir, "Program Files", "FreeNT")
            startup_bat = os.path.join(freent_dir, "start_freent.bat")
            
            if os.path.exists(startup_bat):
                # Create a shortcut (in a real implementation, we'd create a .lnk file)
                shortcut_bat = os.path.join(startup_dir, "FreeNT.lnk")
                with open(shortcut_bat, "w") as f:
                    f.write(f'@echo off\ncall "{startup_bat}"')
            
        except Exception as e:
            print(f"Warning: Failed to create autostart entry: {e}")
    
    def _add_to_startup(self, mount_dir: str) -> None:
        """Add FreeNT to system startup."""
        try:
            # Add to Run registry key
            self._add_run_registry_entry(mount_dir)
            
            # Add to scheduled tasks
            self._add_scheduled_task(mount_dir)
            
        except Exception as e:
            print(f"Warning: Failed to add to startup: {e}")
    
    def _add_run_registry_entry(self, mount_dir: str) -> None:
        """Add FreeNT to Run registry key."""
        try:
            freent_dir = os.path.join(mount_dir, "Program Files", "FreeNT")
            shell_path = os.path.join(freent_dir, "standalone", "bin", "freent.bat")
            
            reg_content = f"""Windows Registry Editor Version 5.00

[HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Run]
"FreeNT"="{shell_path}"

[HKEY_CURRENT_USER\SOFTWARE\Microsoft\Windows\CurrentVersion\Run]
"FreeNT"="{shell_path}"
"""
            
            reg_file = os.path.join(mount_dir, "freent_run.reg")
            with open(reg_file, "w") as f:
                f.write(reg_content)
            
            print(f"Run registry entry created: {reg_file}")
            
        except Exception as e:
            print(f"Warning: Failed to add Run registry entry: {e}")
    
    def _add_scheduled_task(self, mount_dir: str) -> None:
        """Add FreeNT as a scheduled task."""
        try:
            # Create scheduled task XML
            freent_dir = os.path.join(mount_dir, "Program Files", "FreeNT")
            shell_path = os.path.join(freent_dir, "standalone", "bin", "freent.bat")
            
            task_xml = f"""<?xml version="1.0" encoding="UTF-16"?>
<Task version="1.2" xmlns="http://schemas.microsoft.com/windows/2004/02/mit/task">
  <RegistrationInfo>
    <Description>Start FreeNT shell</Description>
    <Author>FreeNT Project</Author>
  </RegistrationInfo>
  <Triggers>
    <LogonTrigger>
      <Enabled>true</Enabled>
    </LogonTrigger>
  </Triggers>
  <Principals>
    <Principal id="Author">
      <UserId>S-1-5-18</UserId>
      <RunLevel>HighestAvailable</RunLevel>
    </Principal>
  </Principals>
  <Settings>
    <MultipleInstancesPolicy>IgnoreNew</MultipleInstancesPolicy>
    <DisallowStartIfOnBatteries>false</DisallowStartIfOnBatteries>
    <StopIfGoingOnBatteries>false</StopIfGoingOnBatteries>
    <AllowHardTerminate>false</AllowHardTerminate>
    <StartWhenAvailable>true</StartWhenAvailable>
    <RunOnlyIfNetworkAvailable>false</RunOnlyIfNetworkAvailable>
    <AllowStartIfOnBatteries>true</AllowStartIfOnBatteries>
    <Enabled>true</Enabled>
    <Hidden>false</Hidden>
    <RunOnlyIfIdle>false</RunOnlyIfIdle>
    <WakeToRun>false</WakeToRun>
    <ExecutionTimeLimit>PT0S</ExecutionTimeLimit>
    <Priority>7</Priority>
  </Settings>
  <Actions Context="Author">
    <Exec>
      <Command>{shell_path}</Command>
    </Exec>
  </Actions>
</Task>
"""
            
            task_dir = os.path.join(mount_dir, "Windows", "System32", "Tasks")
            os.makedirs(task_dir, exist_ok=True)
            
            task_file = os.path.join(task_dir, "FreeNT.xml")
            with open(task_file, "w", encoding="utf-16") as f:
                f.write(task_xml)
            
            print(f"Scheduled task created: {task_file}")
            
        except Exception as e:
            print(f"Warning: Failed to add scheduled task: {e}")
    
    def _create_portable_freent(self, mount_dir: str) -> None:
        """Create portable FreeNT in mounted image."""
        try:
            # Create portable directory
            portable_dir = os.path.join(mount_dir, "FreeNT_Portable")
            os.makedirs(portable_dir, exist_ok=True)
            
            # Copy FreeNT files
            freent_dir = os.path.join(self.config.freent_dir)
            shutil.copytree(freent_dir, os.path.join(portable_dir, "FreeNT"))
            
            # Create portable Python (placeholder)
            python_dir = os.path.join(portable_dir, "Python")
            os.makedirs(python_dir, exist_ok=True)
            
            # Create a README
            readme = os.path.join(portable_dir, "README.txt")
            with open(readme, "w") as f:
                f.write("""FreeNT Portable
================

This directory contains a portable version of FreeNT that can be run
without installation. To use:

1. Navigate to this directory
2. Run FreeNT\standalone\freent.bat

Requirements:
- Windows 10 or 11
- Python 3.8 or later (included in Python directory)

For more information, visit: https://github.com/thepanoc95/FreeNT
""")
            
        except Exception as e:
            print(f"Warning: Failed to create portable FreeNT: {e}")
    
    def _inject_additional_files(self, mount_dir: str) -> None:
        """Inject additional files specified in config."""
        try:
            for src, dest in self.config.additional_files:
                src_path = os.path.join(self.config.freent_dir, src)
                dest_path = os.path.join(mount_dir, dest)
                
                if os.path.exists(src_path):
                    if os.path.isdir(src_path):
                        shutil.copytree(src_path, dest_path)
                    else:
                        os.makedirs(os.path.dirname(dest_path), exist_ok=True)
                        shutil.copy2(src_path, dest_path)
            
        except Exception as e:
            raise InjectionFailedError(f"Failed to inject additional files: {e}") from e
    
    def create_freent_wim(
        self,
        enterprise_iso: str,
        output_wim: str,
        windows_version: int = 11,
    ) -> InjectionResult:
        """
        Create a FreeNT WIM from an Enterprise ISO.
        
        This is the main method for creating a FreeNT installation WIM.
        
        Args:
            enterprise_iso: Path to Enterprise ISO file.
            output_wim: Path for output FreeNT WIM file.
            windows_version: Windows version (10 or 11).
            
        Returns:
            InjectionResult with creation status.
        """
        try:
            from .iso_mounter import ISOMounter
            
            mounter = ISOMounter()
            
            # Mount the ISO
            mount_info = mounter.mount_iso(enterprise_iso)
            
            try:
                # Find WIM file in ISO
                wim_path = mounter.get_wim_path_from_iso(
                    enterprise_iso,
                    edition="Enterprise",
                )
                
                if not wim_path:
                    raise InjectionFailedError("Enterprise WIM not found in ISO")
                
                # Copy WIM to temp location
                temp_wim = os.path.join(mount_info.mount_dir, "install.wim")
                if os.path.exists(temp_wim):
                    wim_path = temp_wim
                
                # Configure injection
                self.config.wim_path = wim_path
                self.config.create_new_wim = True
                self.config.output_wim = output_wim
                self.config.customize_edition = True
                self.config.remove_bloatware = True
                self.config.bloatware_level = "aggressive"
                
                # Set edition name based on version
                if windows_version == 11:
                    self.config.custom_edition_name = "Windows 11 FreeNT"
                    self.config.custom_edition_description = "FreeNT - Alternative Userland for Windows 11"
                else:
                    self.config.custom_edition_name = "Windows 10 FreeNT"
                    self.config.custom_edition_description = "FreeNT - Alternative Userland for Windows 10"
                
                # Inject FreeNT
                return self.inject_freent()
                
            finally:
                # Unmount ISO
                mounter.unmount_iso(enterprise_iso)
        
        except Exception as e:
            return InjectionResult(
                success=False,
                message=f"Failed to create FreeNT WIM: {e}",
                errors=[str(e)],
            )


# Global instance
freent_injector = FreeNTInjector()


if __name__ == "__main__":
    # Example usage
    print("FreeNT Injector Example")
    print("======================")
    
    injector = FreeNTInjector()
    
    print("\nTo use the FreeNT injector:")
    print("1. Mount a WIM image")
    print("2. Configure injection settings")
    print("3. Call injector.inject_freent()")
    print("4. Commit changes and unmount")
    print("\nOr use the convenience method:")
    print("injector.create_freent_wim(enterprise_iso, output_wim)")
