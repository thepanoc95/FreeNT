# FreeNT GNU Toolchain Installer
# Copyright (c) 2026, Panoc95
# BSD 3-Clause License

<#
.SYNOPSIS
    Installs GNU toolchain (gcc, gnumake, etc) for FreeNT on Windows.

.DESCRIPTION
    This script installs the GNU toolchain including:
    - MinGW-w64 (GCC compiler suite)
    - MSYS2 (optional, for Unix-like environment)
    - GNU Make
    - Other essential GNU tools

.NOTES
    File Name      : install_gnu.ps1
    Author         : Panoc95
    Prerequisite   : PowerShell 5.1 or later
    Run as Administrator: Required
#>

param(
    [switch]$InstallMSYS2,
    [switch]$InstallMinGW,
    [switch]$InstallAll = $true,
    [string]$InstallPath = "C:\\FreeNT",
    [switch]$AddToPath = $true,
    [switch]$Force = $false
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

function Install-MinGW {
    param(
        [string]$Path
    )
    
    Write-Header "Installing MinGW-w64"
    
    # Install using Chocolatey
    if (Test-Command choco) {
        Write-Info "Installing MinGW via Chocolatey..."
        try {
            choco install mingw -y --force --no-progress
            Write-Success "MinGW installed via Chocolatey"
            return
        } catch {
            Write-ErrorMsg "Chocolatey installation failed: $_"
        }
    }
    
    # Fallback: Direct download
    Write-Info "Installing MinGW via direct download..."
    
    $mingwUrl = "https://sourceforge.net/projects/mingw-w64/files/Toolchains%20targetting%20Win64/Personal%20Builds/mingw-builds/8.3.0/threads-posix/seh/x86_64-8.3.0-release-posix-seh-rt_v6-rev0.7z"
    $mingwArchive = "$env:TEMP\\mingw.7z"
    $mingwDir = "$Path\\mingw64"
    
    try {
        # Download MinGW
        Write-Info "Downloading MinGW..."
        Invoke-WebRequest -Uri $mingwUrl -OutFile $mingwArchive
        
        # Extract (requires 7-Zip)
        if (-not (Test-Command 7z)) {
            Write-ErrorMsg "7-Zip is required to extract MinGW"
            Write-Info "Installing 7-Zip via Chocolatey..."
            Install-Chocolatey
            choco install 7zip -y --force --no-progress
        }
        
        Write-Info "Extracting MinGW..."
        New-Item -ItemType Directory -Path $mingwDir -Force | Out-Null
        & 7z x $mingwArchive -o"$Path\\mingw64" -y
        
        Write-Success "MinGW installed to $mingwDir"
        
        # Add to PATH
        if ($AddToPath) {
            [Environment]::SetEnvironmentVariable("Path", "$env:Path;$mingwDir\\bin", "Machine")
            Write-Success "Added MinGW to system PATH"
        }
        
    } catch {
        Write-ErrorMsg "Failed to install MinGW: $_"
        exit 1
    }
}

function Install-MSYS2 {
    param(
        [string]$Path
    )
    
    Write-Header "Installing MSYS2"
    
    $msys2Url = "https://github.com/msys2/msys2-installer/releases/download/nightly-x86_64/msys2-base-x86_64-latest.tar.xz"
    $msys2Archive = "$env:TEMP\\msys2.tar.xz"
    $msys2Dir = "$Path\\msys2"
    
    try {
        Write-Info "Downloading MSYS2..."
        Invoke-WebRequest -Uri $msys2Url -OutFile $msys2Archive
        
        Write-Info "Extracting MSYS2..."
        New-Item -ItemType Directory -Path $msys2Dir -Force | Out-Null
        
        # Extract using tar (requires Windows 10+)
        if ($PSVersionTable.PSVersion.Major -ge 5) {
            tar -xf $msys2Archive -C $Path
        } else {
            Write-ErrorMsg "PowerShell 5+ required for tar extraction"
            exit 1
        }
        
        Write-Success "MSYS2 installed to $msys2Dir"
        
        # Initialize MSYS2
        Write-Info "Initializing MSYS2..."
        $msys2Batch = "$msys2Dir\\msys2_shell.cmd"
        if (Test-Path $msys2Batch) {
            # Update packages
            Start-Process -FilePath $msys2Batch -ArgumentList "-mingw64 -defterm -no-start", "pacman -Syu --noconfirm" -Wait
            Start-Process -FilePath $msys2Batch -ArgumentList "-mingw64 -defterm -no-start", "pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-make --noconfirm" -Wait
        }
        
        # Add to PATH
        if ($AddToPath) {
            $msys2Bin = "$msys2Dir\\mingw64\\bin"
            [Environment]::SetEnvironmentVariable("Path", "$env:Path;$msys2Bin", "Machine")
            Write-Success "Added MSYS2 to system PATH"
        }
        
    } catch {
        Write-ErrorMsg "Failed to install MSYS2: $_"
        exit 1
    }
}

function Install-GNUTools {
    param(
        [string]$Path
    )
    
    Write-Header "Installing GNU Tools"
    
    # Install coreutils, findutils, etc. via MSYS2
    if ($InstallMSYS2) {
        Write-Info "Installing GNU tools via MSYS2..."
        $msys2Dir = "$Path\\msys2"
        if (Test-Path "$msys2Dir\\msys2_shell.cmd") {
            $tools = @(
                "mingw-w64-x86_64-coreutils",
                "mingw-w64-x86_64-findutils",
                "mingw-w64-x86_64-gawk",
                "mingw-w64-x86_64-sed",
                "mingw-w64-x86_64-grep",
                "mingw-w64-x86_64-tar",
                "mingw-w64-x86_64-gzip"
            )
            
            $toolList = $tools -join " "
            Start-Process -FilePath "$msys2Dir\\msys2_shell.cmd" -ArgumentList "-mingw64 -defterm -no-start", "pacman -S $toolList --noconfirm" -Wait
            Write-Success "GNU tools installed via MSYS2"
        }
    }
}

function Verify-Installation {
    Write-Header "Verifying Installation"
    
    $requiredCommands = @("gcc", "g++", "make", "gawk", "sed", "grep")
    $missingCommands = @()
    
    foreach ($cmd in $requiredCommands) {
        if (-not (Test-Command $cmd)) {
            $missingCommands += $cmd
        } else {
            Write-Success "Found: $cmd"
        }
    }
    
    if ($missingCommands.Count -gt 0) {
        Write-ErrorMsg "Missing commands: $($missingCommands -join ', ')"
        Write-Info "Some GNU tools may not be available in PATH"
        Write-Info "Try restarting your terminal or computer"
    } else {
        Write-Success "All GNU tools verified successfully!"
    }
}

# Main installation
Write-Header "FreeNT GNU Toolchain Installer"

# Create installation directory
Write-Info "Creating installation directory at $InstallPath..."
New-Item -ItemType Directory -Path $InstallPath -Force | Out-Null

# Install Chocolatey if not available
if (-not (Test-Command choco)) {
    Install-Chocolatey
}

# Install MinGW
if ($InstallMinGW -or $InstallAll) {
    Install-MinGW -Path $InstallPath
}

# Install MSYS2
if ($InstallMSYS2 -or $InstallAll) {
    Install-MSYS2 -Path $InstallPath
}

# Install additional GNU tools
Install-GNUTools -Path $InstallPath

# Verify installation
Verify-Installation

Write-Header "Installation Complete!"
Write-Success "GNU toolchain has been installed for FreeNT"

if ($AddToPath) {
    Write-Info "Please restart your terminal or computer for PATH changes to take effect"
}

# Save configuration
$config = @{
    ToolchainType = "gnu"
    InstallPath = $InstallPath
    MinGWInstalled = $InstallMinGW -or $InstallAll
    MSYS2Installed = $InstallMSYS2 -or $InstallAll
    DateInstalled = Get-Date -Format "yyyy-MM-dd HH:mm:ss"
}

$config | ConvertTo-Json | Out-File "$InstallPath\\gnu-toolchain-config.json"
Write-Success "Configuration saved to $InstallPath\\gnu-toolchain-config.json"
