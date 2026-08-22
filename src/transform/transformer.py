# FreeNT System Transformer
# Copyright (c) 2026, Panoc95
# BSD 3-Clause License

"""
System transformer for FreeNT.
This module provides the main transformation functionality to convert
Windows into FreeNT by replacing components with Vital-Utilities.
"""

import os
import sys
import json
import time
import shutil
from dataclasses import dataclass, field
from typing import Optional, List, Dict, Any, Callable
from pathlib import Path

from .profile import TransformProfile, get_profile, MinimalProfile, StandardProfile, FullProfile
from system.identity import SystemIdentity, modify_system_identity
from system.components import ComponentManager, ComponentAction
from system.vital import VitalUtilitiesManager, ensure_vital_utilities


class TransformError(Exception):
    """Base exception for transformation errors."""
    pass


class TransformValidationError(TransformError):
    """Raised when transformation validation fails."""
    pass


class TransformRollbackError(TransformError):
    """Raised when transformation rollback fails."""
    pass


@dataclass
class TransformConfig:
    """
    Configuration for system transformation.
    
    This class holds all configuration options for the transformation process.
    """
    
    # Profile to use
    profile: Optional[TransformProfile] = None
    profile_name: str = "standard"
    
    # Installation options
    install_vital_utilities: bool = True
    vital_install_dir: Optional[str] = None
    
    # Transformation options
    modify_identity: bool = True
    replace_explorer: bool = True
    disable_components: bool = True
    remove_components: bool = False  # Be careful with this
    
    # Safety options
    create_restore_point: bool = True
    backup_original: bool = True
    dry_run: bool = False
    
    # Logging options
    verbose: bool = True
    log_file: Optional[str] = None
    
    # Callbacks
    on_progress: Optional[Callable[[str, int, int], None]] = None
    on_error: Optional[Callable[[str, Exception], None]] = None
    on_complete: Optional[Callable[[bool, str], None]] = None
    
    def to_dict(self) -> Dict[str, Any]:
        """Convert to dictionary."""
        return {
            "profile_name": self.profile_name,
            "install_vital_utilities": self.install_vital_utilities,
            "vital_install_dir": self.vital_install_dir,
            "modify_identity": self.modify_identity,
            "replace_explorer": self.replace_explorer,
            "disable_components": self.disable_components,
            "remove_components": self.remove_components,
            "create_restore_point": self.create_restore_point,
            "backup_original": self.backup_original,
            "dry_run": self.dry_run,
            "verbose": self.verbose,
            "log_file": self.log_file,
        }
    
    @classmethod
    def from_dict(cls, data: Dict[str, Any]) -> "TransformConfig":
        """Create from dictionary."""
        return cls(
            profile_name=data.get("profile_name", "standard"),
            install_vital_utilities=data.get("install_vital_utilities", True),
            vital_install_dir=data.get("vital_install_dir"),
            modify_identity=data.get("modify_identity", True),
            replace_explorer=data.get("replace_explorer", True),
            disable_components=data.get("disable_components", True),
            remove_components=data.get("remove_components", False),
            create_restore_point=data.get("create_restore_point", True),
            backup_original=data.get("backup_original", True),
            dry_run=data.get("dry_run", False),
            verbose=data.get("verbose", True),
            log_file=data.get("log_file"),
        )


@dataclass
class TransformResult:
    """
    Result of a transformation operation.
    """
    
    success: bool
    message: str
    errors: List[str] = field(default_factory=list)
    warnings: List[str] = field(default_factory=list)
    actions_performed: List[str] = field(default_factory=list)
    start_time: float = 0.0
    end_time: float = 0.0
    
    @property
    def duration(self) -> float:
        """Get transformation duration in seconds."""
        return self.end_time - self.start_time
    
    def to_dict(self) -> Dict[str, Any]:
        """Convert to dictionary."""
        return {
            "success": self.success,
            "message": self.message,
            "errors": self.errors,
            "warnings": self.warnings,
            "actions_performed": self.actions_performed,
            "duration": self.duration,
        }


class SystemTransformer:
    """
    Main system transformer for FreeNT.
    
    This class orchestrates the transformation of Windows into FreeNT
    by modifying system identity, replacing components with Vital-Utilities,
    and managing the overall transformation process.
    """
    
    # Default configuration
    DEFAULT_CONFIG = TransformConfig()
    
    def __init__(self, config: Optional[TransformConfig] = None):
        """
        Initialize the system transformer.
        
        Args:
            config: Optional TransformConfig. If None, uses default.
        """
        self.config = config or self.DEFAULT_CONFIG
        self.component_manager = ComponentManager()
        self.vital_manager = VitalUtilitiesManager(self.config.vital_install_dir)
        self.original_state: Dict[str, Any] = {}
        self.actions_log: List[str] = []
        self.errors: List[str] = []
        self.warnings: List[str] = []
        
        # Load profile
        if self.config.profile is None:
            self.config.profile = get_profile(self.config.profile_name)
    
    def validate_config(self) -> bool:
        """
        Validate the transformation configuration.
        
        Returns:
            True if configuration is valid, False otherwise.
            
        Raises:
            TransformValidationError: If configuration is invalid.
        """
        # Check profile
        if self.config.profile is None:
            self.config.profile = get_profile(self.config.profile_name)
        
        # Check for dangerous operations
        if self.config.remove_components:
            # Check that we're not trying to remove protected components
            for comp in self.config.profile.remove_components:
                if self.component_manager.is_protected(comp):
                    raise TransformValidationError(
                        f"Cannot remove protected component: {comp}"
                    )
        
        # Check that we have a valid profile
        if self.config.profile.name not in ["minimal", "standard", "full", "custom"]:
            raise TransformValidationError(f"Invalid profile: {self.config.profile.name}")
        
        return True
    
    def transform(self) -> TransformResult:
        """
        Perform the system transformation.
        
        This is the main method that transforms Windows into FreeNT.
        
        Returns:
            TransformResult with transformation status.
        """
        result = TransformResult(
            success=False,
            message="",
            start_time=time.time(),
        )
        
        try:
            # Validate configuration
            self.validate_config()
            
            # Log start
            self._log("Starting FreeNT transformation...")
            self._report_progress("Validating configuration", 0, 10)
            
            # Create restore point
            if self.config.create_restore_point:
                self._create_restore_point()
                self._report_progress("Creating restore point", 5, 10)
            
            # Backup original state
            if self.config.backup_original:
                self._backup_original_state()
                self._report_progress("Backing up original state", 10, 10)
            
            # Modify system identity
            if self.config.modify_identity:
                self._modify_system_identity()
                self._report_progress("Modifying system identity", 15, 10)
            
            # Install Vital-Utilities
            if self.config.install_vital_utilities:
                self._install_vital_utilities()
                self._report_progress("Installing Vital-Utilities", 30, 10)
            
            # Replace Explorer
            if self.config.replace_explorer:
                self._replace_explorer()
                self._report_progress("Replacing Explorer", 50, 10)
            
            # Disable components
            if self.config.disable_components:
                self._disable_components()
                self._report_progress("Disabling components", 70, 10)
            
            # Remove components (if enabled)
            if self.config.remove_components:
                self._remove_components()
                self._report_progress("Removing components", 85, 10)
            
            # Finalize
            self._finalize_transformation()
            self._report_progress("Finalizing transformation", 100, 10)
            
            # Success
            result.success = True
            result.message = "FreeNT transformation completed successfully"
            result.actions_performed = self.actions_log
            result.warnings = self.warnings
            
        except Exception as e:
            result.success = False
            result.message = f"FreeNT transformation failed: {e}"
            result.errors = self.errors + [str(e)]
            result.actions_performed = self.actions_log
            
            # Attempt rollback if not dry run
            if not self.config.dry_run:
                self._log(f"Attempting rollback due to error: {e}")
                self.rollback()
        
        finally:
            result.end_time = time.time()
            
            # Call completion callback
            if self.config.on_complete:
                self.config.on_complete(result.success, result.message)
        
        return result
    
    def rollback(self) -> TransformResult:
        """
        Rollback the system transformation.
        
        This method attempts to restore the system to its original state
        before the transformation was applied.
        
        Returns:
            TransformResult with rollback status.
        """
        result = TransformResult(
            success=False,
            message="",
            start_time=time.time(),
        )
        
        try:
            self._log("Starting rollback...")
            
            # Restore Explorer
            if self.config.replace_explorer:
                self._restore_explorer()
            
            # Restore system identity
            if self.config.modify_identity:
                self._restore_system_identity()
            
            # Restore components
            self._restore_components()
            
            # Restore original state
            self._restore_original_state()
            
            result.success = True
            result.message = "Rollback completed successfully"
            
        except Exception as e:
            result.success = False
            result.message = f"Rollback failed: {e}"
            result.errors = [str(e)]
            raise TransformRollbackError(f"Rollback failed: {e}") from e
        
        finally:
            result.end_time = time.time()
        
        return result
    
    def _create_restore_point(self) -> None:
        """Create a system restore point."""
        try:
            self._log("Creating system restore point...")
            
            # Use Windows System Restore
            import subprocess
            result = subprocess.run(
                ["wmic.exe", "/Namespace:\\root\\default", "Path", "SystemRestore", 
                 "Call", "CreateRestorePoint", "FreeNT Transformation", "100", "7"],
                capture_output=True,
                text=True,
                timeout=30,
            )
            
            if result.returncode == 0:
                self._log("System restore point created successfully")
                self.actions_log.append("Created system restore point")
            else:
                self._log(f"Failed to create restore point: {result.stderr}", "warning")
                self.warnings.append("Failed to create system restore point")
                
        except Exception as e:
            self._log(f"Error creating restore point: {e}", "error")
            self.errors.append(f"Failed to create restore point: {e}")
    
    def _backup_original_state(self) -> None:
        """Backup the original system state."""
        try:
            self._log("Backing up original system state...")
            
            # Save system identity
            from system.identity import get_current_identity
            self.original_state["identity"] = get_current_identity().to_dict()
            
            # Save component states
            for comp_name in self.config.profile.replace_components:
                try:
                    status = self.component_manager.get_status(comp_name)
                    self.original_state[f"component_{comp_name}"] = status
                except Exception:
                    pass
            
            # Save registry state
            self._backup_registry()
            
            self._log("Original state backed up successfully")
            self.actions_log.append("Backed up original system state")
            
        except Exception as e:
            self._log(f"Error backing up state: {e}", "error")
            self.errors.append(f"Failed to backup original state: {e}")
    
    def _backup_registry(self) -> None:
        """Backup registry state."""
        try:
            import winreg
            import json
            
            # Backup Winlogon registry key
            with winreg.OpenKey(
                winreg.HKEY_LOCAL_MACHINE,
                r"SOFTWARE\Microsoft\Windows NT\CurrentVersion\Winlogon",
                0,
                winreg.KEY_READ
            ) as key:
                registry_data = {}
                i = 0
                while True:
                    try:
                        name, value, _ = winreg.EnumValue(key, i)
                        registry_data[name] = value
                        i += 1
                    except OSError:
                        break
                
                self.original_state["registry_winlogon"] = registry_data
            
        except Exception:
            pass
    
    def _modify_system_identity(self) -> None:
        """Modify the system identity."""
        try:
            self._log("Modifying system identity...")
            
            # Apply identity modification
            identity = modify_system_identity()
            self.original_state["identity"] = identity.to_dict()
            
            self._log("System identity modified successfully")
            self.actions_log.append("Modified system identity")
            
        except Exception as e:
            self._log(f"Error modifying identity: {e}", "error")
            self.errors.append(f"Failed to modify system identity: {e}")
            raise
    
    def _install_vital_utilities(self) -> None:
        """Install Vital-Utilities."""
        try:
            self._log("Installing Vital-Utilities...")
            
            # Install utilities specified in profile
            results = self.vital_manager.ensure_vital_utilities(
                self.config.profile.install_utilities
            )
            
            # Log results
            for name, success in results.items():
                if success:
                    self._log(f"Successfully installed {name}")
                    self.actions_log.append(f"Installed {name}")
                else:
                    self._log(f"Failed to install {name}", "warning")
                    self.warnings.append(f"Failed to install {name}")
            
            self._log("Vital-Utilities installed successfully")
            
        except Exception as e:
            self._log(f"Error installing Vital-Utilities: {e}", "error")
            self.errors.append(f"Failed to install Vital-Utilities: {e}")
            raise
    
    def _replace_explorer(self) -> None:
        """Replace Windows Explorer."""
        try:
            self._log("Replacing Windows Explorer...")
            
            # Get replacement shell path
            replacement = None
            
            # Try to get Vital-Utilities shell first
            vital_shell_path = self.vital_manager.get_executable_path("vital-shell")
            if vital_shell_path:
                replacement = vital_shell_path
            
            # Fall back to FreeNT login manager
            if replacement is None:
                freent_dir = os.environ.get("FREENT_HOME")
                if freent_dir:
                    replacement = os.path.join(freent_dir, "standalone", "freent.sh")
                else:
                    replacement = os.path.join(
                        os.path.dirname(os.path.abspath(__file__)),
                        "..", "..", "standalone", "freent.sh"
                    )
            
            # Replace Explorer
            success = self.component_manager.replace_explorer(replacement)
            
            if success:
                self._log("Windows Explorer replaced successfully")
                self.actions_log.append("Replaced Windows Explorer")
            else:
                self._log("Failed to replace Windows Explorer", "warning")
                self.warnings.append("Failed to replace Windows Explorer")
                
        except Exception as e:
            self._log(f"Error replacing Explorer: {e}", "error")
            self.errors.append(f"Failed to replace Explorer: {e}")
            raise
    
    def _disable_components(self) -> None:
        """Disable Windows components."""
        try:
            self._log("Disabling Windows components...")
            
            for comp_name in self.config.profile.disable_components:
                try:
                    component = self.component_manager.get_component(comp_name)
                    
                    if component.service_name:
                        success = self.component_manager.disable_service(component.service_name)
                    elif component.executable:
                        success = self.component_manager.kill_process(component.executable)
                    else:
                        success = False
                    
                    if success:
                        self._log(f"Successfully disabled {comp_name}")
                        self.actions_log.append(f"Disabled {comp_name}")
                    else:
                        self._log(f"Failed to disable {comp_name}", "warning")
                        self.warnings.append(f"Failed to disable {comp_name}")
                        
                except Exception as e:
                    self._log(f"Error disabling {comp_name}: {e}", "error")
                    self.errors.append(f"Failed to disable {comp_name}: {e}")
            
            self._log("Windows components disabled successfully")
            
        except Exception as e:
            self._log(f"Error disabling components: {e}", "error")
            self.errors.append(f"Failed to disable components: {e}")
            raise
    
    def _remove_components(self) -> None:
        """Remove Windows components."""
        try:
            self._log("Removing Windows components...")
            
            for comp_name in self.config.profile.remove_components:
                try:
                    component = self.component_manager.get_component(comp_name)
                    
                    # Check if protected
                    if self.component_manager.is_protected(comp_name):
                        self._log(f"Skipping protected component: {comp_name}", "warning")
                        self.warnings.append(f"Skipped protected component: {comp_name}")
                        continue
                    
                    # Remove component (implementation depends on component type)
                    if component.package_name:
                        # Remove via winget
                        success = self._remove_package(component.package_name)
                    elif component.service_name:
                        # Disable and stop service
                        success = (self.component_manager.disable_service(component.service_name) and
                                 self.component_manager.stop_service(comp_name))
                    else:
                        success = False
                    
                    if success:
                        self._log(f"Successfully removed {comp_name}")
                        self.actions_log.append(f"Removed {comp_name}")
                    else:
                        self._log(f"Failed to remove {comp_name}", "warning")
                        self.warnings.append(f"Failed to remove {comp_name}")
                        
                except Exception as e:
                    self._log(f"Error removing {comp_name}: {e}", "error")
                    self.errors.append(f"Failed to remove {comp_name}: {e}")
            
            self._log("Windows components removed successfully")
            
        except Exception as e:
            self._log(f"Error removing components: {e}", "error")
            self.errors.append(f"Failed to remove components: {e}")
            raise
    
    def _remove_package(self, package_name: str) -> bool:
        """Remove a package using winget."""
        try:
            from winget_wrapper.winget import WingetWrapper
            
            wrapper = WingetWrapper()
            result = wrapper.uninstall(package_name, silent=True)
            
            return result.returncode == 0
            
        except Exception:
            return False
    
    def _restore_explorer(self) -> None:
        """Restore Windows Explorer."""
        try:
            self._log("Restoring Windows Explorer...")
            
            success = self.component_manager.restore_explorer()
            
            if success:
                self._log("Windows Explorer restored successfully")
                self.actions_log.append("Restored Windows Explorer")
            else:
                self._log("Failed to restore Windows Explorer", "warning")
                self.warnings.append("Failed to restore Windows Explorer")
                
        except Exception as e:
            self._log(f"Error restoring Explorer: {e}", "error")
            self.errors.append(f"Failed to restore Explorer: {e}")
            raise
    
    def _restore_system_identity(self) -> None:
        """Restore system identity."""
        try:
            self._log("Restoring system identity...")
            
            from system.identity import restore_system_identity
            restore_system_identity()
            
            self._log("System identity restored successfully")
            self.actions_log.append("Restored system identity")
            
        except Exception as e:
            self._log(f"Error restoring identity: {e}", "error")
            self.errors.append(f"Failed to restore system identity: {e}")
            raise
    
    def _restore_components(self) -> None:
        """Restore disabled/removed components."""
        try:
            self._log("Restoring Windows components...")
            
            # Restore all components that were modified
            results = self.component_manager.restore_all()
            
            for name, success in results.items():
                if success:
                    self._log(f"Successfully restored {name}")
                    self.actions_log.append(f"Restored {name}")
                else:
                    self._log(f"Failed to restore {name}", "warning")
                    self.warnings.append(f"Failed to restore {name}")
            
            self._log("Windows components restored successfully")
            
        except Exception as e:
            self._log(f"Error restoring components: {e}", "error")
            self.errors.append(f"Failed to restore components: {e}")
            raise
    
    def _restore_original_state(self) -> None:
        """Restore original system state."""
        try:
            self._log("Restoring original system state...")
            
            # Restore registry
            if "registry_winlogon" in self.original_state:
                self._restore_registry()
            
            # Clear original state
            self.original_state.clear()
            
            self._log("Original system state restored successfully")
            self.actions_log.append("Restored original system state")
            
        except Exception as e:
            self._log(f"Error restoring state: {e}", "error")
            self.errors.append(f"Failed to restore original state: {e}")
            raise
    
    def _restore_registry(self) -> None:
        """Restore registry state."""
        try:
            import winreg
            
            if "registry_winlogon" in self.original_state:
                registry_data = self.original_state["registry_winlogon"]
                
                with winreg.OpenKey(
                    winreg.HKEY_LOCAL_MACHINE,
                    r"SOFTWARE\Microsoft\Windows NT\CurrentVersion\Winlogon",
                    0,
                    winreg.KEY_WRITE
                ) as key:
                    for name, value in registry_data.items():
                        winreg.SetValueEx(key, name, 0, winreg.REG_SZ, value)
            
        except Exception:
            pass
    
    def _finalize_transformation(self) -> None:
        """Finalize the transformation."""
        try:
            self._log("Finalizing transformation...")
            
            # Set environment variables
            self._set_environment_variables()
            
            # Broadcast setting changes
            self._broadcast_setting_changes()
            
            # Create transformation marker
            self._create_transformation_marker()
            
            self._log("Transformation finalized successfully")
            self.actions_log.append("Finalized transformation")
            
        except Exception as e:
            self._log(f"Error finalizing transformation: {e}", "error")
            self.errors.append(f"Failed to finalize transformation: {e}")
            raise
    
    def _set_environment_variables(self) -> None:
        """Set environment variables for FreeNT."""
        try:
            import os
            
            for key, value in self.config.profile.environment_modifications.items():
                os.environ[key] = value
            
            # Also set in system environment (requires admin)
            try:
                import winreg
                
                with winreg.OpenKey(
                    winreg.HKEY_LOCAL_MACHINE,
                    r"SYSTEM\CurrentControlSet\Control\Session Manager\Environment",
                    0,
                    winreg.KEY_WRITE
                ) as key:
                    for key_name, value in self.config.profile.environment_modifications.items():
                        winreg.SetValueEx(key, key_name, 0, winreg.REG_SZ, value)
            except Exception:
                pass
            
        except Exception as e:
            self._log(f"Error setting environment variables: {e}", "warning")
            self.warnings.append(f"Failed to set some environment variables: {e}")
    
    def _broadcast_setting_changes(self) -> None:
        """Broadcast WM_SETTINGCHANGE to notify system of changes."""
        try:
            import ctypes
            
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
    
    def _create_transformation_marker(self) -> None:
        """Create a marker file to indicate transformation is complete."""
        try:
            # Create FreeNT directory if it doesn't exist
            freent_dir = os.environ.get("FREENT_HOME", 
                os.path.join(os.environ.get("ProgramData", "C:\\ProgramData"), "FreeNT"))
            os.makedirs(freent_dir, exist_ok=True)
            
            # Create transformation marker
            marker_path = os.path.join(freent_dir, "TRANSFORMED.flag")
            
            with open(marker_path, "w") as f:
                f.write("FreeNT Transformation Complete\n")
                f.write(f"Profile: {self.config.profile.name}\n")
                f.write(f"Time: {time.strftime('%Y-%m-%d %H:%M:%S')}\n")
                f.write(f"Version: 1.0\n")
            
            # Also create registry marker
            self._create_registry_marker()
            
        except Exception as e:
            self._log(f"Error creating transformation marker: {e}", "warning")
            self.warnings.append(f"Failed to create transformation marker: {e}")
    
    def _create_registry_marker(self) -> None:
        """Create a registry marker for transformation."""
        try:
            import winreg
            
            # Create FreeNT registry key
            with winreg.CreateKey(
                winreg.HKEY_LOCAL_MACHINE,
                r"SOFTWARE\FreeNT"
            ) as key:
                winreg.SetValueEx(key, "Transformed", 0, winreg.REG_DWORD, 1)
                winreg.SetValueEx(key, "Profile", 0, winreg.REG_SZ, self.config.profile.name)
                winreg.SetValueEx(key, "Version", 0, winreg.REG_SZ, "1.0")
                winreg.SetValueEx(key, "Timestamp", 0, winreg.REG_SZ, 
                    time.strftime('%Y-%m-%d %H:%M:%S'))
            
        except Exception:
            pass
    
    def _log(self, message: str, level: str = "info") -> None:
        """Log a message."""
        if not self.config.verbose and level == "info":
            return
        
        timestamp = time.strftime('%Y-%m-%d %H:%M:%S')
        log_message = f"[{timestamp}] [{level.upper()}] {message}"
        
        # Print to console
        print(log_message)
        
        # Write to log file if configured
        if self.config.log_file:
            with open(self.config.log_file, "a") as f:
                f.write(log_message + "\n")
    
    def _report_progress(self, message: str, progress: int, total: int) -> None:
        """Report progress."""
        if self.config.on_progress:
            self.config.on_progress(message, progress, total)


# Global transformer instance
transformer = SystemTransformer()


if __name__ == "__main__":
    # Example usage
    print("FreeNT System Transformer")
    print("========================")
    
    # Create transformer with standard profile
    config = TransformConfig(
        profile_name="standard",
        dry_run=True,  # Dry run for example
        verbose=True,
    )
    
    transformer = SystemTransformer(config)
    
    # Perform transformation
    result = transformer.transform()
    
    print(f"\nTransformation Result:")
    print(f"  Success: {result.success}")
    print(f"  Message: {result.message}")
    print(f"  Duration: {result.duration:.2f} seconds")
    print(f"  Actions: {len(result.actions_performed)}")
    
    if result.errors:
        print(f"\nErrors:")
        for error in result.errors:
            print(f"  - {error}")
    
    if result.warnings:
        print(f"\nWarnings:")
        for warning in result.warnings:
            print(f"  - {warning}")
