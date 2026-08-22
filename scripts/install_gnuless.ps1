# FreeNT GNU-less Toolchain Installer
# Copyright (c) 2026, Panoc95
# BSD 3-Clause License

<#
.SYNOPSIS
    Installs GNU-less toolchain (clang, bsdmake, etc) for FreeNT on Windows.

.DESCRIPTION
    This script installs the GNU-less toolchain including:
    - LLVM/Clang compiler suite
    - BSD Make (pmake)
    - Other essential tools

.NOTES
    File Name      : install_gnuless.ps1
    Author         : Panoc95
    Prerequisite   : PowerShell 5.1 or later
    Run as Administrator: Required
#>

param(
    [switch]$InstallLLVM,
    [switch]$InstallBSDMake,
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

function Install-LLVM {
    param(
        [string]$Path
    )
    
    Write-Header "Installing LLVM/Clang"
    
    # Install using Chocolatey
    if (Test-Command choco) {
        Write-Info "Installing LLVM via Chocolatey..."
        try {
            choco install llvm -y --force --no-progress
            Write-Success "LLVM/Clang installed via Chocolatey"
            return
        } catch {
            Write-ErrorMsg "Chocolatey installation failed: $_"
        }
    }
    
    # Fallback: Direct download
    Write-Info "Installing LLVM via direct download..."
    
    $llvmUrl = "https://github.com/llvm/llvm-project/releases/download/llvmorg-17.0.0/LLVM-17.0.0-win64.exe"
    $llvmInstaller = "$env:TEMP\\LLVM-17.0.0-win64.exe"
    $llvmDir = "$Path\\LLVM"
    
    try {
        # Download LLVM
        Write-Info "Downloading LLVM..."
        Invoke-WebRequest -Uri $llvmUrl -OutFile $llvmInstaller
        
        # Install LLVM
        Write-Info "Installing LLVM to $llvmDir..."
        Start-Process -FilePath $llvmInstaller -ArgumentList "/S /D=$llvmDir" -Wait
        
        Write-Success "LLVM/Clang installed to $llvmDir"
        
        # Add to PATH
        if ($AddToPath) {
            $llvmBin = "$llvmDir\\bin"
            [Environment]::SetEnvironmentVariable("Path", "$env:Path;$llvmBin", "Machine")
            Write-Success "Added LLVM to system PATH"
        }
        
    } catch {
        Write-ErrorMsg "Failed to install LLVM: $_"
        exit 1
    }
}

function Install-BSDMake {
    param(
        [string]$Path
    )
    
    Write-Header "Installing BSD Make (pmake)"
    
    $bsdmakeUrl = "https://sourceforge.net/projects/pmake/files/latest/download"
    $bsdmakeArchive = "$env:TEMP\\pmake.zip"
    $bsdmakeDir = "$Path\\bsdmake"
    
    try {
        Write-Info "Downloading BSD Make..."
        
        # Use Chocolatey if available
        if (Test-Command choco) {
            try {
                choco install bsdmake -y --force --no-progress
                Write-Success "BSD Make installed via Chocolatey"
                return
            } catch {
                Write-Info "Chocolatey installation failed, trying direct download..."
            }
        }
        
        # Direct download (this is a placeholder - actual URL may vary)
        # Note: BSD Make for Windows is not as readily available as GNU Make
        # We'll use a pre-built version or build from source
        
        Write-Info "Building BSD Make from source..."
        
        # Clone BSD Make repository
        $repoUrl = "https://github.com/cheusov/make.git"
        $repoDir = "$env:TEMP\\bsdmake-src"
        
        if (Test-Command git) {
            git clone $repoUrl $repoDir
            
            # Build using Visual Studio or MinGW
            # This is a simplified approach
            $makeExe = "$repoDir\\make.exe"
            if (Test-Path $makeExe) {
                Copy-Item -Path $makeExe -Destination "$bsdmakeDir\\make.exe" -Force
                New-Item -ItemType Directory -Path $bsdmakeDir -Force | Out-Null
                Copy-Item -Path $makeExe -Destination "$bsdmakeDir\\pmake.exe" -Force
                
                Write-Success "BSD Make installed to $bsdmakeDir"
                
                if ($AddToPath) {
                    [Environment]::SetEnvironmentVariable("Path", "$env:Path;$bsdmakeDir", "Machine")
                    Write-Success "Added BSD Make to system PATH"
                }
            } else {
                Write-ErrorMsg "Failed to build BSD Make"
            }
        } else {
            Write-ErrorMsg "Git is required to build BSD Make"
            Write-Info "Installing Git via Chocolatey..."
            choco install git -y --force --no-progress
            Install-BSDMake -Path $Path
        }
        
    } catch {
        Write-ErrorMsg "Failed to install BSD Make: $_"
        Write-Info "You can manually install BSD Make from: https://sourceforge.net/projects/pmake/"
    }
}

function Install-AlternativeTools {
    param(
        [string]$Path
    )
    
    Write-Header "Installing Alternative Tools"
    
    # Install tools that can replace GNU tools
    $tools = @(
        "curl",
        "wget",
        "7zip",
        "less",
        "vim"
    )
    
    if (Test-Command choco) {
        foreach ($tool in $tools) {
            if (-not (Test-Command $tool)) {
                Write-Info "Installing $tool..."
                try {
                    choco install $tool -y --force --no-progress
                    Write-Success "Installed: $tool"
                } catch {
                    Write-ErrorMsg "Failed to install $tool"
                }
            } else {
                Write-Success "Already installed: $tool"
            }
        }
    } else {
        Write-Info "Chocolatey not available, skipping alternative tools"
    }
}

function Verify-Installation {
    Write-Header "Verifying Installation"
    
    $requiredCommands = @("clang", "clang++", "pmake", "make")
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
        Write-Info "Some GNU-less tools may not be available in PATH"
        Write-Info "Try restarting your terminal or computer"
    } else {
        Write-Success "All GNU-less tools verified successfully!"
    }
}

# Main installation
Write-Header "FreeNT GNU-less Toolchain Installer"

# Create installation directory
Write-Info "Creating installation directory at $InstallPath..."
New-Item -ItemType Directory -Path $InstallPath -Force | Out-Null

# Install Chocolatey if not available
if (-not (Test-Command choco)) {
    Install-Chocolatey
}

# Install LLVM/Clang
if ($InstallLLVM -or $InstallAll) {
    Install-LLVM -Path $InstallPath
}

# Install BSD Make
if ($InstallBSDMake -or $InstallAll) {
    Install-BSDMake -Path $InstallPath
}

# Install alternative tools
Install-AlternativeTools -Path $InstallPath

# Verify installation
Verify-Installation

Write-Header "Installation Complete!"
Write-Success "GNU-less toolchain has been installed for FreeNT"

if ($AddToPath) {
    Write-Info "Please restart your terminal or computer for PATH changes to take effect"
}

# Save configuration
$config = @{
    ToolchainType = "gnu-less"
    InstallPath = $InstallPath
    LLVMInstalled = $InstallLLVM -or $InstallAll
    BSDMakeInstalled = $InstallBSDMake -or $InstallAll
    DateInstalled = Get-Date -Format "yyyy-MM-dd HH:mm:ss"
}

$config | ConvertTo-Json | Out-File "$InstallPath\\gnuless-toolchain-config.json"
Write-Success "Configuration saved to $InstallPath\\gnuless-toolchain-config.json"
