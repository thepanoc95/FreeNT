# FreeNT Component Management
# Copyright (c) 2026, Panoc95
# BSD 3-Clause License

"""
Windows component management for FreeNT.
This module handles replacing, disabling, or removing Windows components
and replacing them with Vital-Utilities or FreeNT alternatives.
"""

import os
import sys
import subprocess
import ctypes
from ctypes import wintypes
from typing import Optional, List, Dict, Any, Tuple
from dataclasses import dataclass, field
from enum import Enum


class ComponentAction(str, Enum):
    """Actions that can be performed on components."""
    REPLACE = "replace"
    DISABLE = "disable"
    REMOVE = "remove"
    RESTORE = "restore"
    STATUS = "status"


@dataclass
class WindowsComponent:
    """Represents a Windows component that can be managed."""
    
    name: str
    display_name: str
    description: str
    service_name: Optional[str] = None
    executable: Optional[str] = None
    package_name: Optional[str] = None
    registry_keys: List[str] = field(default_factory=list)
    required: bool = False
    vital_utility: Optional[str] = None
    freent_component: Optional[str] = None
    
    def to_dict(self) -> Dict[str, Any]:
        """Convert to dictionary."""
        return {
            "name": self.name,
            "display_name": self.display_name,
            "description": self.description,
            "service_name": self.service_name,
            "executable": self.executable,
            "package_name": self.package_name,
            "registry_keys": self.registry_keys,
            "required": self.required,
            "vital_utility": self.vital_utility,
            "freent_component": self.freent_component,
        }


class ComponentError(Exception):
    """Base exception for component errors."""
    pass


class ComponentNotFoundError(ComponentError):
    """Raised when a component is not found."""
    pass


class ComponentModifyError(ComponentError):
    """Raised when component modification fails."""
    pass


class ComponentManager:
    """
    Manages Windows components for FreeNT.
    
    This class provides methods to replace, disable, or remove Windows components
    and replace them with Vital-Utilities or FreeNT alternatives.
    """
    
    # List of Windows components that can be managed
    COMPONENTS: Dict[str, WindowsComponent] = {
        "explorer": WindowsComponent(
            name="explorer",
            display_name="Windows Explorer",
            description="Windows shell and file explorer",
            service_name="ShellHWDetection",
            executable="explorer.exe",
            package_name="Microsoft.Windows.Shell.ExperienceHost",
            registry_keys=[
                r"SOFTWARE\Microsoft\Windows NT\CurrentVersion\Winlogon",
                r"SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer",
            ],
            required=False,
            vital_utility="vital-shell",
            freent_component="freent-shell",
        ),
        "taskmgr": WindowsComponent(
            name="taskmgr",
            display_name="Task Manager",
            description="Windows Task Manager",
            executable="taskmgr.exe",
            required=False,
            vital_utility="vital-taskmgr",
            freent_component="freent-taskmgr",
        ),
        "notepad": WindowsComponent(
            name="notepad",
            display_name="Notepad",
            description="Simple text editor",
            executable="notepad.exe",
            required=False,
            vital_utility="vital-notepad",
            freent_component="freent-notepad",
        ),
        "calc": WindowsComponent(
            name="calc",
            display_name="Calculator",
            description="Windows Calculator",
            executable="calc.exe",
            required=False,
            vital_utility="vital-calc",
            freent_component="freent-calc",
        ),
        "paint": WindowsComponent(
            name="paint",
            display_name="Paint",
            description="Simple graphics editor",
            executable="mspaint.exe",
            required=False,
            vital_utility="vital-paint",
            freent_component="freent-paint",
        ),
        "wordpad": WindowsComponent(
            name="wordpad",
            display_name="WordPad",
            description="Rich text editor",
            executable="wordpad.exe",
            required=False,
            vital_utility="vital-wordpad",
            freent_component="freent-wordpad",
        ),
        "cmd": WindowsComponent(
            name="cmd",
            display_name="Command Prompt",
            description="Windows command processor",
            executable="cmd.exe",
            required=False,
            vital_utility="vital-shell",
            freent_component="freent-shell",
        ),
        "powershell": WindowsComponent(
            name="powershell",
            display_name="PowerShell",
            description="Windows PowerShell",
            executable="powershell.exe",
            required=False,
            vital_utility="vital-shell",
            freent_component="freent-shell",
        ),
        "edge": WindowsComponent(
            name="edge",
            display_name="Microsoft Edge",
            description="Web browser",
            executable="msedge.exe",
            package_name="Microsoft.Edge",
            required=False,
            vital_utility="vital-browser",
            freent_component="freent-browser",
        ),
        "iexplore": WindowsComponent(
            name="iexplore",
            display_name="Internet Explorer",
            description="Legacy web browser",
            executable="iexplore.exe",
            required=False,
            vital_utility="vital-browser",
            freent_component="freent-browser",
        ),
        "searchindexer": WindowsComponent(
            name="searchindexer",
            display_name="Windows Search Indexer",
            description="File indexing service",
            service_name="WSearch",
            required=False,
            vital_utility=None,
            freent_component=None,
        ),
        "superfetch": WindowsComponent(
            name="superfetch",
            display_name="Superfetch",
            description="Memory management service",
            service_name="Superfetch",
            required=False,
            vital_utility=None,
            freent_component=None,
        ),
        "windowsupdate": WindowsComponent(
            name="windowsupdate",
            display_name="Windows Update",
            description="Automatic updates service",
            service_name="wuauserv",
            required=False,
            vital_utility=None,
            freent_component=None,
        ),
        "defender": WindowsComponent(
            name="defender",
            display_name="Windows Defender",
            description="Antivirus and security",
            service_name="WinDefend",
            required=False,
            vital_utility="vital-defender",
            freent_component="freent-defender",
        ),
        "cortana": WindowsComponent(
            name="cortana",
            display_name="Cortana",
            description="Virtual assistant",
            service_name=None,
            executable=None,
            package_name="Microsoft.549981C3F5F10",
            required=False,
            vital_utility=None,
            freent_component=None,
        ),
        "onenote": WindowsComponent(
            name="onenote",
            display_name="OneNote",
            description="Note-taking application",
            executable="onenote.exe",
            package_name="Microsoft.Office.OneNote",
            required=False,
            vital_utility="vital-notes",
            freent_component="freent-notes",
        ),
        "skype": WindowsComponent(
            name="skype",
            display_name="Skype",
            description="Communication application",
            executable="skype.exe",
            package_name="Microsoft.SkypeApp",
            required=False,
            vital_utility="vital-chat",
            freent_component="freent-chat",
        ),
        "xbox": WindowsComponent(
            name="xbox",
            display_name="Xbox",
            description="Gaming services",
            service_name="XblAuthManager",
            service_name="XblGameSave",
            service_name="XboxGIpSvc",
            required=False,
            vital_utility=None,
            freent_component=None,
        ),
        # DWM is required - we keep it
        "dwm": WindowsComponent(
            name="dwm",
            display_name="Desktop Window Manager",
            description="Desktop Window Manager Session Manager",
            service_name="UxSms",
            executable="dwm.exe",
            required=True,  # DWM is required
            vital_utility=None,
            freent_component=None,
        ),
        # Critical system components - we keep these
        "csrss": WindowsComponent(
            name="csrss",
            display_name="Client Server Runtime Process",
            description="Critical system component",
            executable="csrss.exe",
            required=True,
            vital_utility=None,
            freent_component=None,
        ),
        "wininit": WindowsComponent(
            name="wininit",
            display_name="Windows Initialization Process",
            description="Critical system component",
            executable="wininit.exe",
            required=True,
            vital_utility=None,
            freent_component=None,
        ),
        "winlogon": WindowsComponent(
            name="winlogon",
            display_name="Windows Logon Application",
            description="Critical system component",
            executable="winlogon.exe",
            required=True,
            vital_utility=None,
            freent_component=None,
        ),
        "services": WindowsComponent(
            name="services",
            display_name="Services and Controller App",
            description="Critical system component",
            executable="services.exe",
            required=True,
            vital_utility=None,
            freent_component=None,
        ),
        "lsass": WindowsComponent(
            name="lsass",
            display_name="Local Security Authority Subsystem Service",
            description="Critical security component",
            executable="lsass.exe",
            required=True,
            vital_utility=None,
            freent_component=None,
        ),
        "svchost": WindowsComponent(
            name="svchost",
            display_name="Service Host",
            description="Host process for Windows services",
            executable="svchost.exe",
            required=True,
            vital_utility=None,
            freent_component=None,
        ),
    }
    
    # Components to disable/remove by default (non-critical)
    DEFAULT_REMOVE_LIST: List[str] = [
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
        "searchindexer",
        "superfetch",
        "windowsupdate",
        "defender",
        "cortana",
        "onenote",
        "skype",
        "xbox",
    ]
    
    # Components that should NEVER be removed (critical for system stability)
    PROTECTED_COMPONENTS: List[str] = [
        "dwm",
        "csrss",
        "wininit",
        "winlogon",
        "services",
        "lsass",
        "svchost",
    ]
    
    def __init__(self):
        """Initialize the component manager."""
        self.modified_components: Dict[str, str] = {}  # component -> action
        self.original_state: Dict[str, Dict[str, Any]] = {}
    
    def list_components(self, show_all: bool = False) -> List[WindowsComponent]:
        """
        List all manageable components.
        
        Args:
            show_all: If True, show all components. If False, show only non-critical.
            
        Returns:
            List of WindowsComponent objects.
        """
        if show_all:
            return list(self.COMPONENTS.values())
        else:
            return [
                comp for comp in self.COMPONENTS.values() 
                if not comp.required
            ]
    
    def get_component(self, name: str) -> WindowsComponent:
        """
        Get a component by name.
        
        Args:
            name: Name of the component.
            
        Returns:
            WindowsComponent object.
            
        Raises:
            ComponentNotFoundError: If component not found.
        """
        if name not in self.COMPONENTS:
            raise ComponentNotFoundError(f"Component '{name}' not found")
        return self.COMPONENTS[name]
    
    def is_protected(self, name: str) -> bool:
        """
        Check if a component is protected (should not be removed).
        
        Args:
            name: Name of the component.
            
        Returns:
            True if component is protected, False otherwise.
        """
        return name in self.PROTECTED_COMPONENTS
    
    def replace_explorer(self, replacement: Optional[str] = None) -> bool:
        """
        Replace Windows Explorer with a custom shell.
        
        This is the main transformation function that replaces Explorer.exe
        with either a Vital-Utilities shell or the FreeNT login manager.
        
        Args:
            replacement: Path to replacement shell. If None, uses FreeNT shell.
            
        Returns:
            True if successful, False otherwise.
        """
        try:
            if replacement is None:
                # Use FreeNT login manager as replacement
                replacement = self._get_freent_shell_path()
            
            # Stop existing Explorer
            self.stop_component("explorer")
            
            # Modify registry to replace shell
            self._modify_shell_registration(replacement)
            
            # Disable Explorer service
            self.disable_service("ShellHWDetection")
            
            # Mark as replaced
            self.modified_components["explorer"] = ComponentAction.REPLACE.value
            
            return True
            
        except Exception as e:
            raise ComponentModifyError(f"Failed to replace Explorer: {e}") from e
    
    def restore_explorer(self) -> bool:
        """
        Restore Windows Explorer.
        
        Returns:
            True if successful, False otherwise.
        """
        try:
            # Restore registry
            self._restore_shell_registration()
            
            # Enable Explorer service
            self.enable_service("ShellHWDetection")
            
            # Start Explorer
            self.start_component("explorer")
            
            # Mark as restored
            if "explorer" in self.modified_components:
                del self.modified_components["explorer"]
            
            return True
            
        except Exception as e:
            raise ComponentModifyError(f"Failed to restore Explorer: {e}") from e
    
    def _get_freent_shell_path(self) -> str:
        """
        Get the path to the FreeNT shell.
        
        Returns:
            Path to FreeNT shell executable.
        """
        # Check if we have a compiled shell
        freent_dir = os.environ.get("FREENT_HOME", os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
        
        # Try different paths
        paths = [
            os.path.join(freent_dir, "bin", "freent-shell.exe"),
            os.path.join(freent_dir, "freent-shell.exe"),
            os.path.join(freent_dir, "src", "login_manager", "login_app.py"),
        ]
        
        for path in paths:
            if os.path.exists(path):
                return path
        
        # Default to Python script
        return os.path.join(freent_dir, "src", "login_manager", "login_app.py")
    
    def _modify_shell_registration(self, shell_path: str) -> None:
        """
        Modify registry to change the shell.
        
        Args:
            shell_path: Path to the replacement shell.
        """
        try:
            import winreg
            
            # Modify Winlogon registry key
            with winreg.OpenKey(
                winreg.HKEY_LOCAL_MACHINE,
                r"SOFTWARE\Microsoft\Windows NT\CurrentVersion\Winlogon",
                0,
                winreg.KEY_WRITE
            ) as key:
                # Save original shell
                try:
                    self.original_state["winlogon_shell"] = winreg.QueryValueEx(key, "Shell")[0]
                except Exception:
                    pass
                
                # Set new shell
                if shell_path.endswith(".py"):
                    # For Python scripts, we need to use python.exe
                    python_exe = sys.executable or "python.exe"
                    shell_cmd = f'"{python_exe}" "{shell_path}"'
                else:
                    shell_cmd = f'"{shell_path}"'
                
                winreg.SetValueEx(key, "Shell", 0, winreg.REG_SZ, shell_cmd)
            
            # Modify Userinit registry key
            try:
                with winreg.OpenKey(
                    winreg.HKEY_LOCAL_MACHINE,
                    r"SOFTWARE\Microsoft\Windows NT\CurrentVersion\Winlogon",
                    0,
                    winreg.KEY_WRITE
                ) as key:
                    try:
                        self.original_state["userinit"] = winreg.QueryValueEx(key, "Userinit")[0]
                    except Exception:
                        pass
                    
                    # Remove Userinit (prevents Explorer from starting)
                    winreg.DeleteValue(key, "Userinit")
            except Exception:
                pass
            
            # Broadcast setting change
            self._broadcast_setting_change()
            
        except ImportError:
            raise ComponentModifyError("winreg module not available (not Windows)")
        except Exception as e:
            raise ComponentModifyError(f"Failed to modify shell registration: {e}") from e
    
    def _restore_shell_registration(self) -> None:
        """Restore the original shell registration."""
        try:
            import winreg
            
            with winreg.OpenKey(
                winreg.HKEY_LOCAL_MACHINE,
                r"SOFTWARE\Microsoft\Windows NT\CurrentVersion\Winlogon",
                0,
                winreg.KEY_WRITE
            ) as key:
                # Restore original shell
                if "winlogon_shell" in self.original_state:
                    winreg.SetValueEx(key, "Shell", 0, winreg.REG_SZ, self.original_state["winlogon_shell"])
                else:
                    winreg.SetValueEx(key, "Shell", 0, winreg.REG_SZ, "explorer.exe")
                
                # Restore Userinit
                if "userinit" in self.original_state:
                    winreg.SetValueEx(key, "Userinit", 0, winreg.REG_SZ, self.original_state["userinit"])
            
            # Broadcast setting change
            self._broadcast_setting_change()
            
        except ImportError:
            pass
        except Exception:
            pass
    
    def _broadcast_setting_change(self) -> None:
        """Broadcast WM_SETTINGCHANGE to notify system of changes."""
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
    
    def stop_component(self, name: str) -> bool:
        """
        Stop a Windows component.
        
        Args:
            name: Name of the component to stop.
            
        Returns:
            True if successful, False otherwise.
        """
        component = self.get_component(name)
        
        if component.service_name:
            return self.stop_service(component.service_name)
        elif component.executable:
            return self.kill_process(component.executable)
        
        return False
    
    def start_component(self, name: str) -> bool:
        """
        Start a Windows component.
        
        Args:
            name: Name of the component to start.
            
        Returns:
            True if successful, False otherwise.
        """
        component = self.get_component(name)
        
        if component.service_name:
            return self.start_service(component.service_name)
        elif component.executable:
            return self.launch_process(component.executable)
        
        return False
    
    def disable_service(self, service_name: str) -> bool:
        """
        Disable a Windows service.
        
        Args:
            service_name: Name of the service to disable.
            
        Returns:
            True if successful, False otherwise.
        """
        try:
            # Use sc command
            result = subprocess.run(
                ["sc", "config", service_name, "start=", "disabled"],
                capture_output=True,
                text=True,
                check=True,
            )
            return result.returncode == 0
        except Exception:
            return False
    
    def enable_service(self, service_name: str) -> bool:
        """
        Enable a Windows service.
        
        Args:
            service_name: Name of the service to enable.
            
        Returns:
            True if successful, False otherwise.
        """
        try:
            # Use sc command
            result = subprocess.run(
                ["sc", "config", service_name, "start=", "auto"],
                capture_output=True,
                text=True,
                check=True,
            )
            return result.returncode == 0
        except Exception:
            return False
    
    def stop_service(self, service_name: str) -> bool:
        """
        Stop a Windows service.
        
        Args:
            service_name: Name of the service to stop.
            
        Returns:
            True if successful, False otherwise.
        """
        try:
            # Use net command
            result = subprocess.run(
                ["net", "stop", service_name],
                capture_output=True,
                text=True,
                check=True,
            )
            return result.returncode == 0
        except Exception:
            return False
    
    def start_service(self, service_name: str) -> bool:
        """
        Start a Windows service.
        
        Args:
            service_name: Name of the service to start.
            
        Returns:
            True if successful, False otherwise.
        """
        try:
            # Use net command
            result = subprocess.run(
                ["net", "start", service_name],
                capture_output=True,
                text=True,
                check=True,
            )
            return result.returncode == 0
        except Exception:
            return False
    
    def kill_process(self, process_name: str) -> bool:
        """
        Kill a running process.
        
        Args:
            process_name: Name of the process to kill.
            
        Returns:
            True if successful, False otherwise.
        """
        try:
            # Use taskkill command
            result = subprocess.run(
                ["taskkill", "/F", "/IM", process_name],
                capture_output=True,
                text=True,
                check=True,
            )
            return result.returncode == 0
        except Exception:
            return False
    
    def launch_process(self, executable: str) -> bool:
        """
        Launch a process.
        
        Args:
            executable: Path to executable to launch.
            
        Returns:
            True if successful, False otherwise.
        """
        try:
            subprocess.Popen([executable], shell=True)
            return True
        except Exception:
            return False
    
    def disable_all_non_critical(self) -> Dict[str, bool]:
        """
        Disable all non-critical Windows components.
        
        Returns:
            Dictionary of component names and success status.
        """
        results = {}
        
        for name in self.DEFAULT_REMOVE_LIST:
            if self.is_protected(name):
                continue
            
            try:
                component = self.get_component(name)
                
                if component.service_name:
                    results[name] = self.disable_service(component.service_name)
                elif component.executable:
                    results[name] = self.kill_process(component.executable)
                else:
                    results[name] = False
                    
            except Exception:
                results[name] = False
        
        return results
    
    def restore_all(self) -> Dict[str, bool]:
        """
        Restore all modified components.
        
        Returns:
            Dictionary of component names and success status.
        """
        results = {}
        
        for name, action in self.modified_components.items():
            try:
                if action == ComponentAction.REPLACE.value:
                    results[name] = self.restore_explorer()
                elif action == ComponentAction.DISABLE.value:
                    component = self.get_component(name)
                    if component.service_name:
                        results[name] = self.enable_service(component.service_name)
                    else:
                        results[name] = False
                elif action == ComponentAction.REMOVE.value:
                    # Removal might require reinstallation
                    results[name] = False
                
            except Exception:
                results[name] = False
        
        self.modified_components.clear()
        return results
    
    def get_status(self, name: str) -> Dict[str, Any]:
        """
        Get the status of a component.
        
        Args:
            name: Name of the component.
            
        Returns:
            Dictionary with component status.
        """
        component = self.get_component(name)
        
        status = {
            "name": component.name,
            "display_name": component.display_name,
            "description": component.description,
            "required": component.required,
            "protected": self.is_protected(name),
        }
        
        # Check if service is running
        if component.service_name:
            status["service_running"] = self._is_service_running(component.service_name)
            status["service_enabled"] = self._is_service_enabled(component.service_name)
        
        # Check if process is running
        if component.executable:
            status["process_running"] = self._is_process_running(component.executable)
        
        # Check modification status
        if name in self.modified_components:
            status["modified"] = True
            status["action"] = self.modified_components[name]
        else:
            status["modified"] = False
        
        return status
    
    def _is_service_running(self, service_name: str) -> bool:
        """Check if a service is running."""
        try:
            result = subprocess.run(
                ["sc", "query", service_name],
                capture_output=True,
                text=True,
                check=True,
            )
            return "RUNNING" in result.stdout
        except Exception:
            return False
    
    def _is_service_enabled(self, service_name: str) -> bool:
        """Check if a service is enabled."""
        try:
            result = subprocess.run(
                ["sc", "qc", service_name],
                capture_output=True,
                text=True,
                check=True,
            )
            return "AUTO_START" in result.stdout or "DEMAND_START" in result.stdout
        except Exception:
            return False
    
    def _is_process_running(self, process_name: str) -> bool:
        """Check if a process is running."""
        try:
            result = subprocess.run(
                ["tasklist", "/FI", f"IMAGENAME eq {process_name}"],
                capture_output=True,
                text=True,
                check=True,
            )
            return process_name in result.stdout
        except Exception:
            return False


# Global instance
component_manager = ComponentManager()


def replace_explorer(replacement: Optional[str] = None) -> bool:
    """
    Replace Windows Explorer with a custom shell.
    
    Args:
        replacement: Path to replacement shell. If None, uses FreeNT shell.
        
    Returns:
        True if successful, False otherwise.
    """
    return component_manager.replace_explorer(replacement)


def restore_explorer() -> bool:
    """
    Restore Windows Explorer.
    
    Returns:
        True if successful, False otherwise.
    """
    return component_manager.restore_explorer()


if __name__ == "__main__":
    # Example usage
    print("Windows Component Manager")
    print("========================")
    
    manager = ComponentManager()
    
    # List all components
    print("\nAll components:")
    for comp in manager.list_components(show_all=True):
        protected = "[PROTECTED]" if manager.is_protected(comp.name) else ""
        print(f"  {comp.name}: {comp.display_name} {protected}")
    
    # Get Explorer status
    print("\nExplorer status:")
    status = manager.get_status("explorer")
    for key, value in status.items():
        print(f"  {key}: {value}")
