# FreeNT Main Installer
# Copyright (c) 2026, Panoc95
# BSD 3-Clause License

<#
.SYNOPSIS
    Main installer for FreeNT - Alternative Userland for Windows NT.

.DESCRIPTION
    This script installs FreeNT with the following components:
    - Python Textual-based Login Manager
    - Winget Wrapper
    - Core utilities
    - Optional: GNU or GNU-less toolchain

.NOTES
    File Name      : install_freent.ps1
    Author         : Panoc95
    Prerequisite   : PowerShell 5.1 or later
    Run as Administrator: Required
#>

param(
    [switch]$InstallGNU,
    [switch]$InstallGNUless,
    [string]$ToolchainType = "gnu",  # "gnu", "gnu-less", or "none"
    [string]$InstallPath = "C:\\FreeNT",
    [switch]$InstallPython = $true,
    [switch]$InstallDependencies = $true,
    [switch]$AddToPath = $true,
    [switch]$Force = $false,
    [switch]$SkipConfirm = $false
)

# Require admin privileges
if (-NOT ([Security.Principal.WindowsPrincipal] [Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole] "Administrator")) {
    Write-Warning "This script must be run as Administrator!"
    exit 1
}

# Set error action preference
$ErrorActionPreference = "Stop"

function Write-Header {
    param([string]$Message)
    Write-Host "`n=== $Message ===`n" -ForegroundColor Cyan
}

function Write-Success {
    param([string]$Message)
    Write-Host "[+] $Message" -ForegroundColor Green
}

function Write-ErrorMsg {
    param([string]$Message)
    Write-Host "[-] $Message" -ForegroundColor Red
}

function Write-Info {
    param([string]$Message)
    Write-Host "[*] $Message" -ForegroundColor Yellow
}

function Test-Command {
    param([string]$Command)
    try {
        $null = Get-Command $Command -ErrorAction Stop
        return $true
    } catch {
        return $false
    }
}

function Confirm-Installation {
    param(
        [string]$ToolchainType
    )
    
    Write-Header "FreeNT Installation Configuration"
    Write-Host "Toolchain Type: $ToolchainType" -ForegroundColor Yellow
    Write-Host "Install Path: $InstallPath" -ForegroundColor Yellow
    Write-Host "Install Python: $InstallPython" -ForegroundColor Yellow
    Write-Host "Install Dependencies: $InstallDependencies" -ForegroundColor Yellow
    Write-Host "Add to PATH: $AddToPath" -ForegroundColor Yellow
    Write-Host ""
    
    if (-not $SkipConfirm) {
        $response = Read-Host "Do you want to proceed with this configuration? (Y/N)"
        if ($response -notmatch "^[yY]") {
            Write-Info "Installation cancelled by user"
            exit 0
        }
    }
}

function Install-Python {
    Write-Header "Installing Python"
    
    if (Test-Command python -or Test-Command python3) {
        Write-Success "Python is already installed"
        return
    }
    
    try {
        Write-Info "Downloading Python installer..."
        $pythonUrl = "https://www.python.org/ftp/python/3.11.0/python-3.11.0-amd64.exe"
        $pythonInstaller = "$env:TEMP\\python-3.11.0-amd64.exe"
        
        Invoke-WebRequest -Uri $pythonUrl -OutFile $pythonInstaller
        
        Write-Info "Installing Python..."
        Start-Process -FilePath $pythonInstaller -ArgumentList "/quiet InstallAllUsers=1 PrependPath=1" -Wait
        
        # Refresh environment
        $env:Path = [System.Environment]::GetEnvironmentVariable("Path","Machine") + ";" + [System.Environment]::GetEnvironmentVariable("Path","User")
        
        Write-Success "Python installed successfully"
    } catch {
        Write-ErrorMsg "Failed to install Python: $_"
        Write-Info "Please install Python manually from https://www.python.org/downloads/"
    }
}

function Install-Chocolatey {
    Write-Header "Installing Chocolatey Package Manager"
    
    if (Test-Command choco) {
        Write-Success "Chocolatey is already installed"
        return
    }
    
    try {
        Write-Info "Downloading Chocolatey installation script..."
        Set-ExecutionPolicy Bypass -Scope Process -Force
        [System.Net.ServicePointManager]::SecurityProtocol = [System.Net.ServicePointManager]::SecurityProtocol -bor 3072
        
        Invoke-Expression ((New-Object System.Net.WebClient).DownloadString('https://chocolatey.org/install.ps1'))
        
        # Refresh environment
        $env:Path = [System.Environment]::GetEnvironmentVariable("Path","Machine") + ";" + [System.Environment]::GetEnvironmentVariable("Path","User")
        
        Write-Success "Chocolatey installed successfully"
    } catch {
        Write-ErrorMsg "Failed to install Chocolatey: $_"
        exit 1
    }
}

function Install-FreeNT-Package {
    Write-Header "Installing FreeNT Package"
    
    # Copy FreeNT files to installation directory
    $repoRoot = $PSScriptRoot
    if (-not $repoRoot) {
        $repoRoot = Get-Location
    }
    
    Write-Info "Copying FreeNT files to $InstallPath..."
    
    try {
        # Create directory structure
        New-Item -ItemType Directory -Path "$InstallPath\\src" -Force | Out-Null
        New-Item -ItemType Directory -Path "$InstallPath\\scripts" -Force | Out-Null
        New-Item -ItemType Directory -Path "$InstallPath\\bin" -Force | Out-Null
        New-Item -ItemType Directory -Path "$InstallPath\\etc" -Force | Out-Null
        New-Item -ItemType Directory -Path "$InstallPath\\var" -Force | Out-Null
        
        # Copy source files
        Copy-Item -Path "$repoRoot\\src" -Destination "$InstallPath\\src" -Recurse -Force
        Copy-Item -Path "$repoRoot\\scripts" -Destination "$InstallPath\\scripts" -Recurse -Force
        Copy-Item -Path "$repoRoot\\*.py" -Destination "$InstallPath" -Force
        Copy-Item -Path "$repoRoot\\*.md" -Destination "$InstallPath" -Force
        Copy-Item -Path "$repoRoot\\*.txt" -Destination "$InstallPath" -Force
        Copy-Item -Path "$repoRoot\\*.cfg" -Destination "$InstallPath" -Force
        Copy-Item -Path "$repoRoot\\*.toml" -Destination "$InstallPath" -Force
        Copy-Item -Path "$repoRoot\\Makefile" -Destination "$InstallPath" -Force
        Copy-Item -Path "$repoRoot\\setup.py" -Destination "$InstallPath" -Force
        Copy-Item -Path "$repoRoot\\LICENSE" -Destination "$InstallPath" -Force
        
        Write-Success "FreeNT files copied to $InstallPath"
        
        # Create batch files for easy access
        $loginScript = @"
@echo off
python "$InstallPath\\src\\login_manager\\login_app.py"
"@
        
        $wingetScript = @"
@echo off
python "$InstallPath\\src\\winget_wrapper\\winget.py"
"@
        
        $freentScript = @"
@echo off
python "$InstallPath\\src\\cli.py"
"@
        
        $loginScript | Out-File -FilePath "$InstallPath\\bin\\freent-login.bat" -Encoding ASCII
        $wingetScript | Out-File -FilePath "$InstallPath\\bin\\freent-winget.bat" -Encoding ASCII
        $freentScript | Out-File -FilePath "$InstallPath\\bin\\freent.bat" -Encoding ASCII
        
        Write-Success "Batch files created in $InstallPath\\bin"
        
        # Add to PATH if requested
        if ($AddToPath) {
            [Environment]::SetEnvironmentVariable("Path", "$env:Path;$InstallPath\\bin", "Machine")
            Write-Success "Added FreeNT bin directory to system PATH"
        }
        
    } catch {
        Write-ErrorMsg "Failed to copy FreeNT files: $_"
        exit 1
    }
}

function Install-Python-Dependencies {
    Write-Header "Installing Python Dependencies"
    
    try {
        # Install pip if not available
        if (-not (Test-Command pip -or Test-Command pip3)) {
            Write-Info "Installing pip..."
            python -m ensurepip --upgrade
        }
        
        # Install requirements
        $repoRoot = $PSScriptRoot
        if (-not $repoRoot) {
            $repoRoot = Get-Location
        }
        
        Write-Info "Installing production dependencies..."
        pip install -r "$repoRoot\\requirements.txt"
        
        Write-Info "Installing development dependencies..."
        pip install -r "$repoRoot\\requirements-dev.txt"
        
        # Install FreeNT in development mode
        Write-Info "Installing FreeNT in development mode..."
        cd "$repoRoot"
        pip install -e .
        
        Write-Success "Python dependencies installed successfully"
    } catch {
        Write-ErrorMsg "Failed to install Python dependencies: $_"
        Write-Info "You can manually install dependencies with: pip install -r requirements.txt"
    }
}

function Install-Toolchain {
    param(
        [string]$Type
    )
    
    Write-Header "Installing $Type Toolchain"
    
    try {
        if ($Type -eq "gnu") {
            Write-Info "Installing GNU toolchain..."
            & "$PSScriptRoot\\install_gnu.ps1" -InstallAll -InstallPath $InstallPath -AddToPath:$AddToPath
        } elseif ($Type -eq "gnu-less") {
            Write-Info "Installing GNU-less toolchain..."
            & "$PSScriptRoot\\install_gnuless.ps1" -InstallAll -InstallPath $InstallPath -AddToPath:$AddToPath
        } else {
            Write-Info "No toolchain selected"
        }
        
        Write-Success "Toolchain installation completed"
    } catch {
        Write-ErrorMsg "Failed to install toolchain: $_"
    }
}

function Create-Configuration {
    Write-Header "Creating FreeNT Configuration"
    
    try {
        $config = @{
            ToolchainType = $ToolchainType
            InstallPath = $InstallPath
            PythonPath = (Get-Command python).Source
            DateInstalled = Get-Date -Format "yyyy-MM-dd HH:mm:ss"
            Version = "0.1.0"
        }
        
        $configDir = "$InstallPath\\etc"
        New-Item -ItemType Directory -Path $configDir -Force | Out-Null
        
        $config | ConvertTo-Json | Out-File -FilePath "$configDir\\freent.json"
        
        Write-Success "Configuration saved to $configDir\\freent.json"
        
    } catch {
        Write-ErrorMsg "Failed to create configuration: $_"
    }
}

function Create-Desktop-Shortcuts {
    Write-Header "Creating Desktop Shortcuts"
    
    try {
        $desktop = [Environment]::GetFolderPath("Desktop")
        $startMenu = [Environment]::GetFolderPath("StartMenu")
        
        # Create WScript.Shell object
        $wshShell = New-Object -ComObject WScript.Shell
        
        # Create shortcut for Login Manager
        $loginShortcut = $wshShell.CreateShortcut("$desktop\\FreeNT Login.lnk")
        $loginShortcut.TargetPath = "$InstallPath\\bin\\freent-login.bat"
        $loginShortcut.WorkingDirectory = "$InstallPath\\bin"
        $loginShortcut.Description = "FreeNT Login Manager"
        $loginShortcut.Save()
        
        # Create shortcut for Winget Wrapper
        $wingetShortcut = $wshShell.CreateShortcut("$desktop\\FreeNT Winget.lnk")
        $wingetShortcut.TargetPath = "$InstallPath\\bin\\freent-winget.bat"
        $wingetShortcut.WorkingDirectory = "$InstallPath\\bin"
        $wingetShortcut.Description = "FreeNT Winget Wrapper"
        $wingetShortcut.Save()
        
        # Create shortcut for FreeNT CLI
        $cliShortcut = $wshShell.CreateShortcut("$desktop\\FreeNT.lnk")
        $cliShortcut.TargetPath = "$InstallPath\\bin\\freent.bat"
        $cliShortcut.WorkingDirectory = "$InstallPath\\bin"
        $cliShortcut.Description = "FreeNT Command Line Interface"
        $cliShortcut.Save()
        
        Write-Success "Desktop shortcuts created"
        
    } catch {
        Write-ErrorMsg "Failed to create desktop shortcuts: $_"
        Write-Info "You can manually create shortcuts to the batch files in $InstallPath\\bin"
    }
}

function Verify-Installation {
    Write-Header "Verifying FreeNT Installation"
    
    $checks = @()
    
    # Check Python
    if (Test-Command python -or Test-Command python3) {
        $checks += @{Name = "Python"; Status = "OK"}
    } else {
        $checks += @{Name = "Python"; Status = "MISSING"}
    }
    
    # Check pip
    if (Test-Command pip -or Test-Command pip3) {
        $checks += @{Name = "Pip"; Status = "OK"}
    } else {
        $checks += @{Name = "Pip"; Status = "MISSING"}
    }
    
    # Check FreeNT files
    if (Test-Path "$InstallPath\\src\\login_manager\\login_app.py") {
        $checks += @{Name = "FreeNT Files"; Status = "OK"}
    } else {
        $checks += @{Name = "FreeNT Files"; Status = "MISSING"}
    }
    
    # Check toolchain
    if ($ToolchainType -eq "gnu") {
        if (Test-Command gcc) {
            $checks += @{Name = "GCC"; Status = "OK"}
        } else {
            $checks += @{Name = "GCC"; Status = "MISSING"}
        }
        if (Test-Command make) {
            $checks += @{Name = "GNU Make"; Status = "OK"}
        } else {
            $checks += @{Name = "GNU Make"; Status = "MISSING"}
        }
    } elseif ($ToolchainType -eq "gnu-less") {
        if (Test-Command clang) {
            $checks += @{Name = "Clang"; Status = "OK"}
        } else {
            $checks += @{Name = "Clang"; Status = "MISSING"}
        }
        if (Test-Command pmake -or Test-Command bsdmake) {
            $checks += @{Name = "BSD Make"; Status = "OK"}
        } else {
            $checks += @{Name = "BSD Make"; Status = "MISSING"}
        }
    }
    
    # Display results
    Write-Host "`nInstallation Status:" -ForegroundColor Cyan
    foreach ($check in $checks) {
        if ($check.Status -eq "OK") {
            Write-Host "  [$($check.Status)] $($check.Name)" -ForegroundColor Green
        } else {
            Write-Host "  [$($check.Status)] $($check.Name)" -ForegroundColor Red
        }
    }
    
    $allOk = $true
    foreach ($check in $checks) {
        if ($check.Status -ne "OK") {
            $allOk = $false
            break
        }
    }
    
    if ($allOk) {
        Write-Success "All checks passed!"
    } else {
        Write-ErrorMsg "Some checks failed. Please review the output above."
    }
    
    return $allOk
}

function Display-Summary {
    param(
        [bool]$Success
    )
    
    Write-Header "Installation Summary"
    Write-Host "FreeNT has been installed to: $InstallPath" -ForegroundColor Yellow
    Write-Host "Toolchain Type: $ToolchainType" -ForegroundColor Yellow
    Write-Host "Python: $(if (Test-Command python) { (python --version 2>&1) } else { 'Not installed' })" -ForegroundColor Yellow
    
    if ($Success) {
        Write-Host "`nInstallation completed successfully!" -ForegroundColor Green
        Write-Host "`nYou can now:" -ForegroundColor Cyan
        Write-Host "  - Run the login manager: freent-login" -ForegroundColor Yellow
        Write-Host "  - Use the winget wrapper: freent-winget" -ForegroundColor Yellow
        Write-Host "  - Access FreeNT CLI: freent" -ForegroundColor Yellow
        
        if ($AddToPath) {
            Write-Host "`nNote: You may need to restart your computer for PATH changes to take effect." -ForegroundColor Yellow
        }
    } else {
        Write-Host "`nInstallation completed with some issues." -ForegroundColor Red
        Write-Host "Please review the output above and fix any problems." -ForegroundColor Yellow
    }
}

# Main installation process
Write-Header "FreeNT Installer"
Write-Host "Welcome to FreeNT - Alternative Userland for Windows NT" -ForegroundColor Cyan
Write-Host "Version: 0.1.0" -ForegroundColor Cyan
Write-Host ""

# Determine toolchain type
if ($InstallGNU) {
    $ToolchainType = "gnu"
} elseif ($InstallGNUless) {
    $ToolchainType = "gnu-less"
}

# Confirm installation
Confirm-Installation -ToolchainType $ToolchainType

# Install prerequisites
if ($InstallPython) {
    Install-Python
}

# Install Chocolatey (for package management)
Install-Chocolatey

# Install FreeNT package
Install-FreeNT-Package

# Install Python dependencies
if ($InstallDependencies) {
    Install-Python-Dependencies
}

# Install toolchain
Install-Toolchain -Type $ToolchainType

# Create configuration
Create-Configuration

# Create desktop shortcuts
Create-Desktop-Shortcuts

# Verify installation
$installSuccess = Verify-Installation

# Display summary
Display-Summary -Success $installSuccess

# Save installation log
$log = @{
    Timestamp = Get-Date -Format "yyyy-MM-dd HH:mm:ss"
    ToolchainType = $ToolchainType
    InstallPath = $InstallPath
    PythonInstalled = (Test-Command python -or Test-Command python3)
    ChocolateyInstalled = (Test-Command choco)
    Success = $installSuccess
    Errors = $errorCount
}

$log | ConvertTo-Json | Out-File -FilePath "$InstallPath\\install-log.json"

Write-Success "Installation log saved to $InstallPath\\install-log.json"

# Final message
if ($installSuccess) {
    Write-Host "`nFreeNT is ready to use!" -ForegroundColor Green
} else {
    Write-Host "`nFreeNT installation needs attention. Please check the log file." -ForegroundColor Red
}
