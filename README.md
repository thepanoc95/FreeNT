# FreeNT

An Free and Open-Source alternative userland for Windows NT-series operating systems (Windows 10 and Windows 11, with 32-bit support).

## Features

- **Custom Login Manager**: Built with Python Textual, providing a modern and customizable login experience
- **Winget Wrapper**: A convenient Python interface to Microsoft's winget package manager
- **Dual Toolchain Support**: Works with both GNU (gcc, gnumake) and GNU-less (clang, bsdmake) toolchains
- **Flexible Installation**: Multiple installation options and configurations
- **Open Source**: Licensed under BSD 3-Clause License

## Requirements

- **Operating System**: Windows 10 or Windows 11 (Windows 11 recommended)
- **Architecture**: x64, x86, ARM64 (x64 recommended)
- **Python**: Python 3.8 or later
- **PowerShell**: PowerShell 5.1 or later (for installation scripts)

## Installation

### Quick Install

1. **Clone the repository**:
   ```powershell
   git clone https://github.com/thepanoc95/FreeNT.git
   cd FreeNT
   ```

2. **Run the installer** (as Administrator):
   ```powershell
   .\scripts\install_freent.ps1
   ```

3. **Follow the prompts** to configure your installation.

### Manual Installation

1. **Install Python** (if not already installed):
   - Download from [python.org](https://www.python.org/downloads/)
   - Make sure to check "Add Python to PATH" during installation

2. **Install dependencies**:
   ```powershell
   pip install -r requirements.txt
   ```

3. **Install in development mode**:
   ```powershell
   pip install -e .
   ```

4. **Install toolchain** (choose one):
   - **GNU Toolchain**:
     ```powershell
     .\scripts\install_gnu.ps1
     ```
   - **GNU-less Toolchain**:
     ```powershell
     .\scripts\install_gnuless.ps1
     ```

## Usage

### Command Line Interface

FreeNT provides a CLI with the following commands:

```bash
# Show version
freent --version

# Show system information
freent info

# Start login manager
freent login

# Winget wrapper commands
freent winget search python
freent winget install Python.Python.3.11
freent winget uninstall Python.Python.3.11
freent winget list
freent winget upgrade

# Configuration commands
freent config show
freent config set toolchain.type gnu
freent config get toolchain.type
```

### Login Manager

Start the graphical login manager:

```bash
freent login
```

Or run directly:

```bash
python -m src.login_manager.login_app
```

### Winget Wrapper

Use the winget wrapper for package management:

```python
from src.winget_wrapper import WingetWrapper

wrapper = WingetWrapper()

# Search for packages
results = wrapper.search("python", limit=5)
for pkg in results:
    print(f"{pkg.id} - {pkg.name} ({pkg.version})")

# Install a package
wrapper.install("Python.Python.3.11")

# List installed packages
installed = wrapper.list_installed()
for pkg in installed:
    print(f"{pkg.id} - {pkg.name} ({pkg.version})")
```

## Project Structure

```
FreeNT/
├── src/
│   ├── __init__.py
│   ├── cli.py                 # Command Line Interface
│   ├── core/
│   │   ├── __init__.py
│   │   ├── config.py          # Configuration management
│   │   └── utils.py           # Utility functions
│   ├── login_manager/
│   │   ├── __init__.py
│   │   └── login_app.py       # Textual-based login manager
│   └── winget_wrapper/
│       ├── __init__.py
│       └── winget.py           # Winget wrapper implementation
├── scripts/
│   ├── __init__.py
│   ├── install_freent.ps1     # Main installer
│   ├── install_gnu.ps1        # GNU toolchain installer
│   └── install_gnuless.ps1    # GNU-less toolchain installer
├── installers/
├── build/
├── docs/
├── tests/
├── Makefile                  # Build system (GNU/BSD compatible)
├── setup.py                  # Setup script
├── setup.cfg                 # Setup configuration
├── pyproject.toml            # Project configuration
├── requirements.txt          # Production dependencies
├── requirements-dev.txt      # Development dependencies
├── README.md
└── LICENSE
```

## Configuration

FreeNT uses a JSON-based configuration system. The configuration file is typically located at:

- Windows: `%APPDATA%\FreeNT\config.json`
- Other: `~/.config/FreeNT/config.json`

### Configuration Options

```json
{
  "config_version": "1.0",
  "windows_version": "11",
  "architecture": "x64",
  "toolchain": {
    "type": "gnu",
    "gcc_path": null,
    "gnumake_path": null,
    "clang_path": null,
    "bsdmake_path": null,
    "python_path": null
  },
  "login_manager": {
    "enabled": true,
    "theme": "light",
    "auto_login": false,
    "auto_login_user": null,
    "show_shutdown_button": true,
    "show_reboot_button": true
  },
  "winget": {
    "enabled": true,
    "auto_accept_agreements": false,
    "preferred_source": "winget",
    "cache_dir": null
  },
  "debug_mode": false,
  "log_level": "INFO"
}
```

## Toolchain Support

### GNU Toolchain

The GNU toolchain includes:
- **GCC** (GNU Compiler Collection)
- **GNU Make**
- **Coreutils** (GNU core utilities)
- **Findutils**
- **Gawk**
- **Sed**
- **Grep**
- **Tar**
- **Gzip**

Installation:
```powershell
.\scripts\install_gnu.ps1
```

### GNU-less Toolchain

The GNU-less toolchain includes:
- **LLVM/Clang** compiler suite
- **BSD Make** (pmake)
- Alternative tools (curl, wget, 7zip, etc.)

Installation:
```powershell
.\scripts\install_gnuless.ps1
```

## Build System

FreeNT uses a Makefile that is compatible with both GNU Make and BSD Make.

### Build Targets

```bash
# Build everything
make all

# Install production dependencies
make install-deps

# Install development dependencies
make install-dev-deps

# Run tests
make test
make test-unit
make test-integration

# Run linter
make lint

# Format code
make format

# Run type checker
make check-types

# Clean build artifacts
make clean
make clean-all

# Install FreeNT
make install

# Create distribution packages
make package

# Show help
make help
```

## Development

### Setting Up Development Environment

1. Clone the repository:
   ```bash
   git clone https://github.com/thepanoc95/FreeNT.git
   cd FreeNT
   ```

2. Create a virtual environment:
   ```bash
   python -m venv venv
   source venv/bin/activate  # On Windows: venv\Scripts\activate
   ```

3. Install development dependencies:
   ```bash
   pip install -r requirements.txt
   pip install -r requirements-dev.txt
   pip install -e .
   ```

4. Run tests:
   ```bash
   pytest
   ```

### Contributing

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/your-feature`)
3. Commit your changes (`git commit -am 'Add some feature'`)
4. Push to the branch (`git push origin feature/your-feature`)
5. Open a Pull Request

### Code Style

- **Line Length**: 88 characters
- **Formatting**: Black
- **Import Sorting**: isort
- **Linting**: flake8
- **Type Checking**: mypy

## License

FreeNT is licensed under the BSD 3-Clause License. See the [LICENSE](LICENSE) file for details.

## Support

- **Issues**: [GitHub Issues](https://github.com/thepanoc95/FreeNT/issues)
- **Discussions**: [GitHub Discussions](https://github.com/thepanoc95/FreeNT/discussions)
- **Documentation**: [GitHub Wiki](https://github.com/thepanoc95/FreeNT/wiki)

## Roadmap

- [ ] Complete login manager functionality
- [ ] Add more winget wrapper features
- [ ] Implement system service management
- [ ] Add package repository support
- [ ] Create documentation
- [ ] Add localization support
- [ ] Implement themes and customization
- [ ] Add accessibility features
- [ ] Create installer GUI
- [ ] Add update mechanism

## Acknowledgments

- **Python Textual**: For the excellent TUI framework
- **Microsoft**: For Windows and winget
- **MinGW-w64**: For GNU tools on Windows
- **LLVM**: For Clang compiler
- **All contributors**: For their valuable contributions

---

**FreeNT** - Making Windows Free and Open Source!

Copyright (c) 2026, Panoc95
