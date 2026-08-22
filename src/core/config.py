# FreeNT Core - Configuration Management
# Copyright (c) 2026, Panoc95
# BSD 3-Clause License

"""
Configuration management for FreeNT.
This module handles loading, saving, and managing FreeNT configuration.
"""

import json
import os
import sys
from dataclasses import dataclass, field, asdict
from pathlib import Path
from typing import Optional, Dict, Any, List


class ConfigError(Exception):
    """Base exception for configuration errors."""
    pass


class ConfigFileNotFoundError(ConfigError):
    """Raised when configuration file is not found."""
    pass


class ConfigValidationError(ConfigError):
    """Raised when configuration is invalid."""
    pass


@dataclass
class ToolchainConfig:
    """Configuration for toolchain settings."""
    type: str = "gnu"  # "gnu" or "gnu-less"
    gcc_path: Optional[str] = None
    gnumake_path: Optional[str] = None
    clang_path: Optional[str] = None
    bsdmake_path: Optional[str] = None
    python_path: Optional[str] = None
    
    def validate(self) -> None:
        """Validate toolchain configuration."""
        if self.type not in ["gnu", "gnu-less"]:
            raise ConfigValidationError(
                f"Invalid toolchain type: {self.type}. Must be 'gnu' or 'gnu-less'."
            )
        
        if self.type == "gnu":
            if not self.gcc_path and not self._find_in_path("gcc"):
                raise ConfigValidationError(
                    "GNU toolchain selected but gcc not found in PATH or configured."
                )
            if not self.gnumake_path and not self._find_in_path("make"):
                raise ConfigValidationError(
                    "GNU toolchain selected but make not found in PATH or configured."
                )
        
        if self.type == "gnu-less":
            if not self.clang_path and not self._find_in_path("clang"):
                raise ConfigValidationError(
                    "GNU-less toolchain selected but clang not found in PATH or configured."
                )
            if not self.bsdmake_path and not self._find_in_path("bsdmake"):
                raise ConfigValidationError(
                    "GNU-less toolchain selected but bsdmake not found in PATH or configured."
                )
    
    @staticmethod
    def _find_in_path(executable: str) -> bool:
        """Check if executable is in PATH."""
        try:
            import shutil
            return shutil.which(executable) is not None
        except ImportError:
            # Fallback for systems without shutil.which
            paths = os.getenv("PATH", "").split(os.pathsep)
            for path in paths:
                full_path = os.path.join(path, executable)
                if os.path.exists(full_path) and os.access(full_path, os.X_OK):
                    return True
            return False


@dataclass
class LoginManagerConfig:
    """Configuration for login manager."""
    enabled: bool = True
    theme: str = "light"  # "light" or "dark"
    auto_login: bool = False
    auto_login_user: Optional[str] = None
    show_shutdown_button: bool = True
    show_reboot_button: bool = True
    
    def validate(self) -> None:
        """Validate login manager configuration."""
        if self.theme not in ["light", "dark"]:
            raise ConfigValidationError(
                f"Invalid theme: {self.theme}. Must be 'light' or 'dark'."
            )
        
        if self.auto_login and not self.auto_login_user:
            raise ConfigValidationError(
                "Auto login enabled but no user specified."
            )


@dataclass
class WingetConfig:
    """Configuration for winget wrapper."""
    enabled: bool = True
    auto_accept_agreements: bool = False
    preferred_source: str = "winget"  # "winget", "msstore", or "all"
    cache_dir: Optional[str] = None
    
    def validate(self) -> None:
        """Validate winget configuration."""
        if self.preferred_source not in ["winget", "msstore", "all"]:
            raise ConfigValidationError(
                f"Invalid preferred source: {self.preferred_source}. "
                "Must be 'winget', 'msstore', or 'all'."
            )


@dataclass
class FreeNTConfig:
    """
    Main configuration for FreeNT.
    
    This class holds all configuration settings for FreeNT and provides
    methods for loading, saving, and validating configuration.
    """
    
    # Version of configuration schema
    config_version: str = "1.0"
    
    # Core settings
    windows_version: str = "11"  # Detected automatically
    architecture: str = "x64"  # Detected automatically
    
    # Toolchain configuration
    toolchain: ToolchainConfig = field(default_factory=ToolchainConfig)
    
    # Login manager configuration
    login_manager: LoginManagerConfig = field(default_factory=LoginManagerConfig)
    
    # Winget configuration
    winget: WingetConfig = field(default_factory=WingetConfig)
    
    # Additional settings
    debug_mode: bool = False
    log_level: str = "INFO"  # "DEBUG", "INFO", "WARNING", "ERROR", "CRITICAL"
    
    def __post_init__(self):
        """Initialize default values after creation."""
        # Detect Windows version if not set
        if self.windows_version == "11":
            self.windows_version = self._detect_windows_version()
        
        # Detect architecture if not set
        if self.architecture == "x64":
            self.architecture = self._detect_architecture()
    
    @staticmethod
    def _detect_windows_version() -> str:
        """Detect Windows version."""
        try:
            import platform
            version = platform.win32_ver()
            major = version[0]
            minor = version[1]
            
            if major == 10 and minor >= 22000:  # Windows 11
                return "11"
            elif major == 10:  # Windows 10
                return "10"
            else:
                return f"{major}.{minor}"
        except Exception:
            return "11"  # Default to 11
    
    @staticmethod
    def _detect_architecture() -> str:
        """Detect system architecture."""
        try:
            import platform
            machine = platform.machine()
            if machine in ["AMD64", "x86_64"]:
                return "x64"
            elif machine in ["x86", "i386", "i686"]:
                return "x86"
            elif machine == "ARM64":
                return "arm64"
            else:
                return machine.lower()
        except Exception:
            return "x64"  # Default to x64
    
    def validate(self) -> None:
        """Validate the entire configuration."""
        self.toolchain.validate()
        self.login_manager.validate()
        self.winget.validate()
        
        if self.log_level not in ["DEBUG", "INFO", "WARNING", "ERROR", "CRITICAL"]:
            raise ConfigValidationError(
                f"Invalid log level: {self.log_level}. "
                "Must be one of: DEBUG, INFO, WARNING, ERROR, CRITICAL."
            )
    
    def to_dict(self) -> Dict[str, Any]:
        """Convert configuration to dictionary."""
        return {
            "config_version": self.config_version,
            "windows_version": self.windows_version,
            "architecture": self.architecture,
            "toolchain": asdict(self.toolchain),
            "login_manager": asdict(self.login_manager),
            "winget": asdict(self.winget),
            "debug_mode": self.debug_mode,
            "log_level": self.log_level,
        }
    
    @classmethod
    def from_dict(cls, data: Dict[str, Any]) -> "FreeNTConfig":
        """Create configuration from dictionary."""
        return cls(
            config_version=data.get("config_version", "1.0"),
            windows_version=data.get("windows_version", "11"),
            architecture=data.get("architecture", "x64"),
            toolchain=ToolchainConfig(**data.get("toolchain", {})),
            login_manager=LoginManagerConfig(**data.get("login_manager", {})),
            winget=WingetConfig(**data.get("winget", {})),
            debug_mode=data.get("debug_mode", False),
            log_level=data.get("log_level", "INFO"),
        )
    
    def save(self, path: Optional[str] = None) -> None:
        """
        Save configuration to file.
        
        Args:
            path: Path to save configuration. If None, uses default config path.
        """
        if path is None:
            path = self.get_default_config_path()
        
        try:
            with open(path, "w", encoding="utf-8") as f:
                json.dump(self.to_dict(), f, indent=2)
        except Exception as e:
            raise ConfigError(f"Failed to save configuration: {e}") from e
    
    @classmethod
    def load(cls, path: Optional[str] = None) -> "FreeNTConfig":
        """
        Load configuration from file.
        
        Args:
            path: Path to load configuration from. If None, uses default config path.
            
        Returns:
            FreeNTConfig instance with loaded settings.
        """
        if path is None:
            path = cls.get_default_config_path()
        
        if not os.path.exists(path):
            raise ConfigFileNotFoundError(f"Configuration file not found: {path}")
        
        try:
            with open(path, "r", encoding="utf-8") as f:
                data = json.load(f)
            
            config = cls.from_dict(data)
            config.validate()
            return config
        except json.JSONDecodeError as e:
            raise ConfigError(f"Failed to parse configuration file: {e}") from e
        except ConfigValidationError as e:
            raise ConfigError(f"Invalid configuration: {e}") from e
        except Exception as e:
            raise ConfigError(f"Failed to load configuration: {e}") from e
    
    @classmethod
    def load_or_default(cls, path: Optional[str] = None) -> "FreeNTConfig":
        """
        Load configuration from file or return default.
        
        Args:
            path: Path to load configuration from. If None, uses default config path.
            
        Returns:
            FreeNTConfig instance with loaded settings or defaults.
        """
        try:
            return cls.load(path)
        except ConfigFileNotFoundError:
            # Create default configuration
            config = cls()
            config.save(path)
            return config
    
    @staticmethod
    def get_default_config_path() -> str:
        """
        Get the default configuration file path.
        
        Returns:
            Path to default configuration file.
        """
        # Use AppData/LocalAppData for Windows
        if sys.platform == "win32":
            import os
            appdata = os.getenv("APPDATA") or os.path.expanduser("~/AppData/Roaming")
            config_dir = os.path.join(appdata, "FreeNT")
            os.makedirs(config_dir, exist_ok=True)
            return os.path.join(config_dir, "config.json")
        else:
            # For non-Windows systems (testing)
            home = Path.home()
            config_dir = home / ".config" / "FreeNT"
            config_dir.mkdir(parents=True, exist_ok=True)
            return str(config_dir / "config.json")


# Global configuration instance
_config: Optional[FreeNTConfig] = None


def get_config() -> FreeNTConfig:
    """
    Get the global configuration instance.
    
    Returns:
        Global FreeNTConfig instance.
    """
    global _config
    if _config is None:
        _config = FreeNTConfig.load_or_default()
    return _config


def set_config(config: FreeNTConfig) -> None:
    """
    Set the global configuration instance.
    
    Args:
        config: FreeNTConfig instance to set as global.
    """
    global _config
    _config = config


def reset_config() -> None:
    """Reset the global configuration instance."""
    global _config
    _config = None
