# FreeNT Command Line Interface
# Copyright (c) 2026, Panoc95
# BSD 3-Clause License

"""
Command Line Interface for FreeNT.
This module provides a CLI for managing FreeNT functionality.
"""

import argparse
import sys
import json
from typing import Optional, List, Dict, Any


def main():
    """Main entry point for FreeNT CLI."""
    parser = argparse.ArgumentParser(
        prog="freent",
        description="FreeNT - Alternative Userland for Windows NT",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  freent --version
  freent info
  freent login
  freent winget search python
  freent winget install Python.Python.3.11
  freent config show
  freent config set toolchain gnu
        """
    )
    
    # Add version argument
    parser.add_argument(
        "--version",
        action="store_true",
        help="Show version information and exit"
    )
    
    # Subcommands
    subparsers = parser.add_subparsers(
        dest="command",
        title="commands",
        description="Available commands"
    )
    
    # Info command
    info_parser = subparsers.add_parser(
        "info",
        help="Show system and FreeNT information"
    )
    info_parser.set_defaults(func=cmd_info)
    
    # Login command
    login_parser = subparsers.add_parser(
        "login",
        help="Start the FreeNT login manager"
    )
    login_parser.set_defaults(func=cmd_login)
    
    # Winget command
    winget_parser = subparsers.add_parser(
        "winget",
        help="Manage packages using winget wrapper"
    )
    winget_subparsers = winget_parser.add_subparsers(
        dest="winget_command",
        title="winget commands",
        description="Winget wrapper commands"
    )
    
    # Winget search
    winget_search_parser = winget_subparsers.add_parser(
        "search",
        help="Search for packages"
    )
    winget_search_parser.add_argument(
        "query",
        help="Search query"
    )
    winget_search_parser.add_argument(
        "--source",
        choices=["winget", "msstore", "all"],
        default="all",
        help="Package source to search"
    )
    winget_search_parser.add_argument(
        "--exact",
        action="store_true",
        help="Perform exact match search"
    )
    winget_search_parser.add_argument(
        "--limit",
        type=int,
        default=10,
        help="Maximum number of results"
    )
    winget_search_parser.set_defaults(func=cmd_winget_search)
    
    # Winget install
    winget_install_parser = winget_subparsers.add_parser(
        "install",
        help="Install a package"
    )
    winget_install_parser.add_argument(
        "package_id",
        help="Package identifier to install"
    )
    winget_install_parser.add_argument(
        "--version",
        help="Specific version to install"
    )
    winget_install_parser.add_argument(
        "--source",
        choices=["winget", "msstore"],
        help="Package source"
    )
    winget_install_parser.add_argument(
        "--silent",
        action="store_true",
        help="Install silently"
    )
    winget_install_parser.set_defaults(func=cmd_winget_install)
    
    # Winget uninstall
    winget_uninstall_parser = winget_subparsers.add_parser(
        "uninstall",
        help="Uninstall a package"
    )
    winget_uninstall_parser.add_argument(
        "package_id",
        help="Package identifier to uninstall"
    )
    winget_uninstall_parser.add_argument(
        "--version",
        help="Specific version to uninstall"
    )
    winget_uninstall_parser.add_argument(
        "--silent",
        action="store_true",
        help="Uninstall silently"
    )
    winget_uninstall_parser.set_defaults(func=cmd_winget_uninstall)
    
    # Winget list
    winget_list_parser = winget_subparsers.add_parser(
        "list",
        help="List installed packages"
    )
    winget_list_parser.set_defaults(func=cmd_winget_list)
    
    # Winget upgrade
    winget_upgrade_parser = winget_subparsers.add_parser(
        "upgrade",
        help="Upgrade packages"
    )
    winget_upgrade_parser.add_argument(
        "package_id",
        nargs="?",
        help="Package identifier to upgrade (default: all)"
    )
    winget_upgrade_parser.set_defaults(func=cmd_winget_upgrade)
    
    # Config command
    config_parser = subparsers.add_parser(
        "config",
        help="Manage FreeNT configuration"
    )
    config_subparsers = config_parser.add_subparsers(
        dest="config_command",
        title="config commands",
        description="Configuration commands"
    )
    
    # Config show
    config_show_parser = config_subparsers.add_parser(
        "show",
        help="Show current configuration"
    )
    config_show_parser.set_defaults(func=cmd_config_show)
    
    # Config set
    config_set_parser = config_subparsers.add_parser(
        "set",
        help="Set configuration value"
    )
    config_set_parser.add_argument(
        "key",
        help="Configuration key to set"
    )
    config_set_parser.add_argument(
        "value",
        help="Value to set"
    )
    config_set_parser.set_defaults(func=cmd_config_set)
    
    # Config get
    config_get_parser = config_subparsers.add_parser(
        "get",
        help="Get configuration value"
    )
    config_get_parser.add_argument(
        "key",
        help="Configuration key to get"
    )
    config_get_parser.set_defaults(func=cmd_config_get)
    
    # Parse arguments
    args = parser.parse_args()
    
    # Handle version
    if args.version:
        print("FreeNT v0.1.0")
        print("Alternative Userland for Windows NT")
        print("Copyright (c) 2026, Panoc95")
        print("License: BSD 3-Clause")
        return 0
    
    # Handle no command
    if args.command is None:
        parser.print_help()
        return 0
    
    # Call the appropriate function
    try:
        return args.func(args)
    except Exception as e:
        print(f"Error: {e}", file=sys.stderr)
        return 1


def cmd_info(args) -> int:
    """Show system and FreeNT information."""
    import platform
    import os
    
    print("FreeNT Information")
    print("==================")
    print(f"Version: 0.1.0")
    print(f"Author: Panoc95")
    print(f"License: BSD 3-Clause")
    print()
    
    print("System Information")
    print("==================")
    print(f"Platform: {platform.platform()}")
    print(f"System: {platform.system()}")
    print(f"Release: {platform.release()}")
    print(f"Version: {platform.version()}")
    print(f"Machine: {platform.machine()}")
    print(f"Processor: {platform.processor()}")
    print()
    
    print("Python Information")
    print("==================")
    print(f"Python: {sys.version}")
    print(f"Executable: {sys.executable}")
    print(f"Path: {sys.path}")
    print()
    
    # Check for FreeNT components
    print("FreeNT Components")
    print("==================")
    
    # Check login manager
    try:
        from login_manager.login_app import LoginApp
        print("Login Manager: Available")
    except ImportError:
        print("Login Manager: Not available")
    
    # Check winget wrapper
    try:
        from winget_wrapper.winget import WingetWrapper
        print("Winget Wrapper: Available")
    except ImportError:
        print("Winget Wrapper: Not available")
    
    # Check core
    try:
        from core.config import FreeNTConfig
        print("Core Configuration: Available")
    except ImportError:
        print("Core Configuration: Not available")
    
    return 0


def cmd_login(args) -> int:
    """Start the FreeNT login manager."""
    try:
        from login_manager.login_app import LoginApp
        app = LoginApp()
        result = app.run()
        if result:
            username, password = result
            print(f"Login successful: {username}")
        return 0
    except ImportError as e:
        print(f"Error: Failed to import login manager: {e}", file=sys.stderr)
        print("Make sure FreeNT is installed correctly.")
        return 1
    except Exception as e:
        print(f"Error: {e}", file=sys.stderr)
        return 1


def cmd_winget_search(args) -> int:
    """Search for packages using winget wrapper."""
    try:
        from winget_wrapper.winget import WingetWrapper, PackageSource
        
        wrapper = WingetWrapper()
        
        source = PackageSource(args.source) if hasattr(args, 'source') else PackageSource.ALL
        results = wrapper.search(
            query=args.query,
            source=source,
            exact=args.exact if hasattr(args, 'exact') else False,
            limit=args.limit if hasattr(args, 'limit') else 10
        )
        
        if not results:
            print("No packages found.")
            return 0
        
        print(f"Found {len(results)} packages:\n")
        for i, pkg in enumerate(results, 1):
            print(f"{i}. {pkg.id}")
            print(f"   Name: {pkg.name}")
            print(f"   Version: {pkg.version}")
            print(f"   Source: {pkg.source}")
            print(f"   Publisher: {pkg.publisher}")
            if pkg.description:
                print(f"   Description: {pkg.description}")
            print()
        
        return 0
    except ImportError as e:
        print(f"Error: Failed to import winget wrapper: {e}", file=sys.stderr)
        return 1
    except Exception as e:
        print(f"Error: {e}", file=sys.stderr)
        return 1


def cmd_winget_install(args) -> int:
    """Install a package using winget wrapper."""
    try:
        from winget_wrapper.winget import WingetWrapper, PackageSource
        
        wrapper = WingetWrapper()
        
        source = PackageSource(args.source) if hasattr(args, 'source') and args.source else None
        silent = args.silent if hasattr(args, 'silent') else False
        
        print(f"Installing {args.package_id}...", end=" ", flush=True)
        
        result = wrapper.install(
            package_id=args.package_id,
            version=args.version if hasattr(args, 'version') and args.version else None,
            source=source,
            silent=silent,
            accept_agreements=True
        )
        
        if result.returncode == 0:
            print("Done!")
            return 0
        else:
            print(f"Failed with code {result.returncode}")
            if result.stderr:
                print(f"Error: {result.stderr}")
            return 1
    except ImportError as e:
        print(f"Error: Failed to import winget wrapper: {e}", file=sys.stderr)
        return 1
    except Exception as e:
        print(f"Error: {e}", file=sys.stderr)
        return 1


def cmd_winget_uninstall(args) -> int:
    """Uninstall a package using winget wrapper."""
    try:
        from winget_wrapper.winget import WingetWrapper, PackageSource
        
        wrapper = WingetWrapper()
        
        silent = args.silent if hasattr(args, 'silent') else False
        
        print(f"Uninstalling {args.package_id}...", end=" ", flush=True)
        
        result = wrapper.uninstall(
            package_id=args.package_id,
            version=args.version if hasattr(args, 'version') and args.version else None,
            silent=silent
        )
        
        if result.returncode == 0:
            print("Done!")
            return 0
        else:
            print(f"Failed with code {result.returncode}")
            if result.stderr:
                print(f"Error: {result.stderr}")
            return 1
    except ImportError as e:
        print(f"Error: Failed to import winget wrapper: {e}", file=sys.stderr)
        return 1
    except Exception as e:
        print(f"Error: {e}", file=sys.stderr)
        return 1


def cmd_winget_list(args) -> int:
    """List installed packages using winget wrapper."""
    try:
        from winget_wrapper.winget import WingetWrapper
        
        wrapper = WingetWrapper()
        packages = wrapper.list_installed()
        
        if not packages:
            print("No packages installed.")
            return 0
        
        print(f"Installed packages ({len(packages)}):\n")
        for i, pkg in enumerate(packages, 1):
            print(f"{i}. {pkg.id} ({pkg.version})")
            print(f"   Source: {pkg.source}")
            print()
        
        return 0
    except ImportError as e:
        print(f"Error: Failed to import winget wrapper: {e}", file=sys.stderr)
        return 1
    except Exception as e:
        print(f"Error: {e}", file=sys.stderr)
        return 1


def cmd_winget_upgrade(args) -> int:
    """Upgrade packages using winget wrapper."""
    try:
        from winget_wrapper.winget import WingetWrapper
        
        wrapper = WingetWrapper()
        
        package_id = args.package_id if hasattr(args, 'package_id') and args.package_id else None
        
        if package_id:
            print(f"Upgrading {package_id}...", end=" ", flush=True)
        else:
            print("Upgrading all packages...", end=" ", flush=True)
        
        result = wrapper.upgrade(package_id)
        
        if result.returncode == 0:
            print("Done!")
            return 0
        else:
            print(f"Failed with code {result.returncode}")
            if result.stderr:
                print(f"Error: {result.stderr}")
            return 1
    except ImportError as e:
        print(f"Error: Failed to import winget wrapper: {e}", file=sys.stderr)
        return 1
    except Exception as e:
        print(f"Error: {e}", file=sys.stderr)
        return 1


def cmd_config_show(args) -> int:
    """Show current configuration."""
    try:
        from core.config import get_config
        
        config = get_config()
        
        print("FreeNT Configuration")
        print("====================")
        print(f"Config Version: {config.config_version}")
        print(f"Windows Version: {config.windows_version}")
        print(f"Architecture: {config.architecture}")
        print(f"Debug Mode: {config.debug_mode}")
        print(f"Log Level: {config.log_level}")
        print()
        
        print("Toolchain Configuration")
        print("----------------------")
        print(f"Type: {config.toolchain.type}")
        if config.toolchain.gcc_path:
            print(f"GCC Path: {config.toolchain.gcc_path}")
        if config.toolchain.gnumake_path:
            print(f"GNU Make Path: {config.toolchain.gnumake_path}")
        if config.toolchain.clang_path:
            print(f"Clang Path: {config.toolchain.clang_path}")
        if config.toolchain.bsdmake_path:
            print(f"BSD Make Path: {config.toolchain.bsdmake_path}")
        if config.toolchain.python_path:
            print(f"Python Path: {config.toolchain.python_path}")
        print()
        
        print("Login Manager Configuration")
        print("---------------------------")
        print(f"Enabled: {config.login_manager.enabled}")
        print(f"Theme: {config.login_manager.theme}")
        print(f"Auto Login: {config.login_manager.auto_login}")
        if config.login_manager.auto_login_user:
            print(f"Auto Login User: {config.login_manager.auto_login_user}")
        print(f"Show Shutdown Button: {config.login_manager.show_shutdown_button}")
        print(f"Show Reboot Button: {config.login_manager.show_reboot_button}")
        print()
        
        print("Winget Configuration")
        print("--------------------")
        print(f"Enabled: {config.winget.enabled}")
        print(f"Auto Accept Agreements: {config.winget.auto_accept_agreements}")
        print(f"Preferred Source: {config.winget.preferred_source}")
        if config.winget.cache_dir:
            print(f"Cache Directory: {config.winget.cache_dir}")
        
        return 0
    except ImportError as e:
        print(f"Error: Failed to import configuration: {e}", file=sys.stderr)
        return 1
    except Exception as e:
        print(f"Error: {e}", file=sys.stderr)
        return 1


def cmd_config_set(args) -> int:
    """Set configuration value."""
    try:
        from core.config import get_config, set_config
        import json
        
        config = get_config()
        
        # Parse the key path (e.g., "toolchain.type" or "login_manager.theme")
        parts = args.key.split('.')
        
        # Navigate to the parent object
        obj = config
        for part in parts[:-1]:
            obj = getattr(obj, part)
        
        # Set the value
        last_part = parts[-1]
        
        # Convert value type
        value = args.value
        
        # Try to convert to appropriate type
        if value.lower() in ['true', 'false']:
            value = value.lower() == 'true'
        elif value.isdigit():
            value = int(value)
        elif value.replace('.', '').replace('-', '').isdigit():
            value = float(value)
        
        setattr(obj, last_part, value)
        
        # Save configuration
        set_config(config)
        config.save()
        
        print(f"Configuration '{args.key}' set to '{value}'")
        return 0
    except ImportError as e:
        print(f"Error: Failed to import configuration: {e}", file=sys.stderr)
        return 1
    except AttributeError as e:
        print(f"Error: Invalid configuration key: {e}", file=sys.stderr)
        return 1
    except Exception as e:
        print(f"Error: {e}", file=sys.stderr)
        return 1


def cmd_config_get(args) -> int:
    """Get configuration value."""
    try:
        from core.config import get_config
        
        config = get_config()
        
        # Parse the key path
        parts = args.key.split('.')
        
        # Navigate to the value
        obj = config
        for part in parts:
            obj = getattr(obj, part)
        
        print(obj)
        return 0
    except ImportError as e:
        print(f"Error: Failed to import configuration: {e}", file=sys.stderr)
        return 1
    except AttributeError as e:
        print(f"Error: Invalid configuration key: {e}", file=sys.stderr)
        return 1
    except Exception as e:
        print(f"Error: {e}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
