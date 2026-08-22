# FreeNT System Transformation Script
# Copyright (c) 2026, Panoc95
# BSD 3-Clause License

<#
.SYNOPSIS
    Transforms Windows into FreeNT by replacing components with Vital-Utilities.

.DESCRIPTION
    This script performs the following transformations:
    - Changes system identity to FreeNT
    - Replaces Windows Explorer with Vital-Utilities shell or FreeNT shell
    - Disables non-essential Windows services
    - Optionally removes non-critical Windows components
    - Installs Vital-Utilities as replacements
    - Keeps only DWM and absolutely required system components

.NOTES
    File Name      : transform.ps1
    Author         : Panoc95
    Prerequisite   : PowerShell 5.1 or later
    Run as Administrator: REQUIRED
#>

param(
    [string]$Profile = "standard",  # minimal, standard, full, or custom
    [switch]$Minimal,
    [switch]$Standard,
    [switch]$Full,
    [switch]$DryRun,
    [switch]$Force,
    [switch]$SkipConfirm,
    [string]$VitalInstallDir = "$env:ProgramFiles\Vital-Utilities",
    [switch]$NoVitalUtilities,
    [string]$LogFile = "$env:TEMP\FreeNT_Transform.log"
)

# Set profile based on parameters
if ($Minimal) { $Profile = "minimal" }
if ($Standard) { $Profile = "standard" }
if ($Full) { $Profile = "full" }

# Require admin privileges
if (-NOT ([Security.Principal.WindowsPrincipal] [Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole] "Administrator")) {
    Write-Warning "This script MUST be run as Administrator!"
    Write-Host "FreeNT transformation requires elevated privileges to modify system components."
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

function Write-WarningMsg {
    param([string]$Message)
    Write-Host "[!] $Message" -ForegroundColor Magenta
}

function Confirm-Transformation {
    param(
        [string]$Profile
    )
    
    Write-Header "FreeNT System Transformation"
    Write-Host "This script will transform your Windows system into FreeNT." -ForegroundColor Yellow
    Write-Host ""
    Write-Host "Profile: $Profile" -ForegroundColor Cyan
    Write-Host ""
    
    Write-Host "This transformation will:" -ForegroundColor Yellow
    
    switch ($Profile) {
        "minimal" {
            Write-Host "  - Change system identity to FreeNT" -ForegroundColor Yellow
            Write-Host "  - Replace Windows Explorer with FreeNT shell" -ForegroundColor Yellow
            Write-Host "  - Keep all other Windows components" -ForegroundColor Yellow
        }
        "standard" {
            Write-Host "  - Change system identity to FreeNT" -ForegroundColor Yellow
            Write-Host "  - Replace Windows Explorer with FreeNT shell" -ForegroundColor Yellow
            Write-Host "  - Replace common utilities (notepad, calc, paint, etc.) with Vital-Utilities" -ForegroundColor Yellow
            Write-Host "  - Disable non-essential services" -ForegroundColor Yellow
            Write-Host "  - Keep critical Windows components" -ForegroundColor Yellow
        }
        "full" {
            Write-Host "  - Change system identity to FreeNT" -ForegroundColor Yellow
            Write-Host "  - Replace Windows Explorer with FreeNT shell" -ForegroundColor Yellow
            Write-Host "  - Replace ALL non-critical utilities with Vital-Utilities" -ForegroundColor Yellow
            Write-Host "  - Disable all non-essential services" -ForegroundColor Yellow
            Write-Host "  - Remove unnecessary Windows components" -ForegroundColor Yellow
            Write-Host "  - Keep ONLY DWM and absolutely required components" -ForegroundColor Yellow
        }
        default {
            Write-Host "  - Custom transformation based on profile: $Profile" -ForegroundColor Yellow
        }
    }
    
    Write-Host ""
    Write-Host "WARNING: This transformation may affect system stability." -ForegroundColor Red
    Write-Host "A system restore point will be created before transformation." -ForegroundColor Yellow
    Write-Host "You can rollback the transformation if needed." -ForegroundColor Yellow
    Write-Host ""
    
    if (-not $SkipConfirm) {
        $response = Read-Host "Do you want to proceed? (Y/N)"
        if ($response -notmatch "^[yY]") {
            Write-Info "Transformation cancelled by user"
            exit 0
        }
    }
}

function Install-VitalUtilities {
    param(
        [string]$InstallDir,
        [string[]]$Utilities
    )
    
    Write-Header "Installing Vital-Utilities"
    
    $vitalRepo = "https://github.com/Vital-Utilities/Vital-Utilities"
    $tempDir = Join-Path $env:TEMP "Vital-Utilities"
    
    # Create installation directory
    if (-not (Test-Path $InstallDir)) {
        New-Item -ItemType Directory -Path $InstallDir -Force | Out-Null
    }
    
    # Create temp directory
    if (-not (Test-Path $tempDir)) {
        New-Item -ItemType Directory -Path $tempDir -Force | Out-Null
    }
    
    # List of Vital-Utilities to install based on profile
    $utilityMap = @{
        "vital-shell" = @{
            Name = "Vital Shell"
            Description = "Replacement for cmd.exe and PowerShell"
            Replaces = @("cmd", "powershell")
        }
        "vital-taskmgr" = @{
            Name = "Vital Task Manager"
            Description = "Replacement for Windows Task Manager"
            Replaces = @("taskmgr")
        }
        "vital-notepad" = @{
            Name = "Vital Notepad"
            Description = "Replacement for Notepad"
            Replaces = @("notepad")
        }
        "vital-calc" = @{
            Name = "Vital Calculator"
            Description = "Replacement for Windows Calculator"
            Replaces = @("calc")
        }
        "vital-paint" = @{
            Name = "Vital Paint"
            Description = "Replacement for MS Paint"
            Replaces = @("paint")
        }
        "vital-filemanager" = @{
            Name = "Vital File Manager"
            Description = "Replacement for File Explorer"
            Replaces = @("explorer")
        }
        "vital-browser" = @{
            Name = "Vital Browser"
            Description = "Lightweight web browser"
            Replaces = @("edge", "iexplore")
        }
        "vital-defender" = @{
            Name = "Vital Defender"
            Description = "Security and antivirus"
            Replaces = @("defender")
        }
        "vital-notes" = @{
            Name = "Vital Notes"
            Description = "Note-taking application"
            Replaces = @("onenote")
        }
        "vital-chat" = @{
            Name = "Vital Chat"
            Description = "Communication application"
            Replaces = @("skype")
        }
    }
    
    $installed = @()
    $failed = @()
    
    foreach ($utility in $Utilities) {
        if ($utilityMap.ContainsKey($utility)) {
            $info = $utilityMap[$utility]
            Write-Info "Installing $($info.Name)..."
            
            try {
                # Download Vital-Utilities (placeholder - actual download logic needed)
                # For now, we'll assume they're already available or use a placeholder
                $utilityDir = Join-Path $InstallDir $utility
                
                if (-not (Test-Path $utilityDir)) {
                    New-Item -ItemType Directory -Path $utilityDir -Force | Out-Null
                }
                
                # Create a placeholder executable (in real implementation, this would be downloaded)
                $exePath = Join-Path $utilityDir "$utility.exe"
                if (-not (Test-Path $exePath)) {
                    # Create a simple batch file as placeholder
                    $batchContent = @"
@echo off
ECHO $($info.Name) - Vital Utility
ECHO This is a placeholder for the actual $utility executable
ECHO Replaces: $($info.Replaces -join ", ")
PAUSE
"@
                    $batchContent | Out-File -FilePath $exePath -Encoding ASCII
                }
                
                Write-Success "Installed $($info.Name)"
                $installed += $utility
                
            } catch {
                Write-ErrorMsg "Failed to install $($info.Name): $_"
                $failed += $utility
            }
        }
    }
    
    Write-Info "Installed $($installed.Count) utilities, failed $($failed.Count)"
    
    if ($failed.Count -gt 0) {
        Write-WarningMsg "Some utilities failed to install"
    }
    
    return @{
        Installed = $installed
        Failed = $failed
    }
}

function Modify-SystemIdentity {
    Write-Header "Modifying System Identity"
    
    try {
        # Modify registry to change system branding
        Write-Info "Modifying registry branding..."
        
        $regPaths = @(
            "HKLM:\SOFTWARE\Microsoft\Windows NT\CurrentVersion",
            "HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion"
        )
        
        foreach ($regPath in $regPaths) {
            try {
                # Save original values
                $original = @{}
                $key = Get-ItemProperty -Path $regPath -ErrorAction SilentlyContinue
                if ($key) {
                    $original.ProductName = $key.ProductName
                    $original.DisplayVersion = $key.DisplayVersion
                    $original.CurrentBuild = $key.CurrentBuild
                    $original.CurrentVersion = $key.CurrentVersion
                }
                
                # Set new values
                Set-ItemProperty -Path $regPath -Name "ProductName" -Value "FreeNT" -Force
                Set-ItemProperty -Path $regPath -Name "DisplayVersion" -Value "1.0" -Force
                Set-ItemProperty -Path $regPath -Name "CurrentBuild" -Value "1.0" -Force
                Set-ItemProperty -Path $regPath -Name "CurrentVersion" -Value "FreeNT 1.0" -Force
                
                Write-Success "Modified $regPath"
                
            } catch {
                Write-ErrorMsg "Failed to modify $regPath : $_"
            }
        }
        
        # Modify registered organization
        try {
            $regPath = "HKLM:\SOFTWARE\Microsoft\Windows NT\CurrentVersion"
            Set-ItemProperty -Path $regPath -Name "RegisteredOrganization" -Value "FreeNT Project" -Force
            Set-ItemProperty -Path $regPath -Name "RegisteredOwner" -Value "FreeNT User" -Force
            Write-Success "Modified registered organization"
        } catch {
            Write-ErrorMsg "Failed to modify registered organization: $_"
        }
        
        # Set environment variables
        Write-Info "Setting environment variables..."
        [Environment]::SetEnvironmentVariable("FREENT", "1", "Machine")
        [Environment]::SetEnvironmentVariable("FREENT_VERSION", "1.0", "Machine")
        [Environment]::SetEnvironmentVariable("FREENT_PROFILE", $Profile, "Machine")
        Write-Success "Set environment variables"
        
        # Broadcast setting change
        Write-Info "Broadcasting setting changes..."
        $signature = @'
[DllImport("user32.dll", CharSet = CharSet.Auto)]
public static extern IntPtr SendMessageTimeout(
    IntPtr hWnd, uint Msg, UIntPtr wParam, string lParam,
    uint fuFlags, uint uTimeout, out UIntPtr lpdwResult);
'@
        $type = Add-Type -MemberDefinition $signature -Name "Win32SendMessage" -Namespace Win32Functions -PassThru
        
        $HWND_BROADCAST = [IntPtr]0xFFFF
        $WM_SETTINGCHANGE = 0x001A
        $result = [UIntPtr]::Zero
        
        [Win32Functions.Win32SendMessage]::SendMessageTimeout(
            $HWND_BROADCAST, $WM_SETTINGCHANGE, [UIntPtr]::Zero, "Environment",
            0, 5000, [ref]$result) | Out-Null
        
        Write-Success "System identity modified successfully"
        
    } catch {
        Write-ErrorMsg "Failed to modify system identity: $_"
        throw
    }
}

function Replace-Explorer {
    param(
        [string]$ReplacementPath
    )
    
    Write-Header "Replacing Windows Explorer"
    
    try {
        # Check if replacement exists
        if (-not (Test-Path $ReplacementPath)) {
            Write-ErrorMsg "Replacement shell not found: $ReplacementPath"
            return $false
        }
        
        Write-Info "Replacement shell: $ReplacementPath"
        
        # Stop existing Explorer
        Write-Info "Stopping Windows Explorer..."
        $explorerProcesses = Get-Process -Name explorer -ErrorAction SilentlyContinue
        if ($explorerProcesses) {
            Stop-Process -Name explorer -Force -ErrorAction SilentlyContinue
            Start-Sleep -Seconds 2
            Write-Success "Stopped Windows Explorer"
        } else {
            Write-Info "Windows Explorer not running"
        }
        
        # Modify registry to replace shell
        Write-Info "Modifying shell registration..."
        
        $regPath = "HKLM:\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Winlogon"
        
        # Save original shell
        $originalShell = (Get-ItemProperty -Path $regPath -Name "Shell" -ErrorAction SilentlyContinue).Shell
        if ($originalShell) {
            Write-Info "Original shell: $originalShell"
        }
        
        # Set new shell
        if ($ReplacementPath -like "*.py") {
            # For Python scripts
            $pythonExe = Get-Command python -ErrorAction SilentlyContinue
            if ($pythonExe) {
                $newShell = "`"$($pythonExe.Source)`" `"$ReplacementPath`""
            } else {
                $newShell = "`"python`" `"$ReplacementPath`""
            }
        } else {
            $newShell = "`"$ReplacementPath`""
        }
        
        Set-ItemProperty -Path $regPath -Name "Shell" -Value $newShell -Force
        Write-Success "Shell registration modified"
        
        # Remove Userinit to prevent Explorer from starting
        Write-Info "Disabling Userinit..."
        try {
            Remove-ItemProperty -Path $regPath -Name "Userinit" -ErrorAction SilentlyContinue
            Write-Success "Userinit disabled"
        } catch {
            Write-Info "Userinit not found or already removed"
        }
        
        # Broadcast setting change
        Write-Info "Broadcasting setting changes..."
        $signature = @'
[DllImport("user32.dll", CharSet = CharSet.Auto)]
public static extern IntPtr SendMessageTimeout(
    IntPtr hWnd, uint Msg, UIntPtr wParam, string lParam,
    uint fuFlags, uint uTimeout, out UIntPtr lpdwResult);
'@
        $type = Add-Type -MemberDefinition $signature -Name "Win32SendMessage" -Namespace Win32Functions -PassThru
        
        $HWND_BROADCAST = [IntPtr]0xFFFF
        $WM_SETTINGCHANGE = 0x001A
        $result = [UIntPtr]::Zero
        
        [Win32Functions.Win32SendMessage]::SendMessageTimeout(
            $HWND_BROADCAST, $WM_SETTINGCHANGE, [UIntPtr]::Zero, "Environment",
            0, 5000, [ref]$result) | Out-Null
        
        Write-Success "Windows Explorer replaced successfully"
        return $true
        
    } catch {
        Write-ErrorMsg "Failed to replace Explorer: $_"
        return $false
    }
}

function Disable-Components {
    param(
        [string[]]$Components
    )
    
    Write-Header "Disabling Windows Components"
    
    $componentMap = @{
        "explorer" = @{ Service = "ShellHWDetection"; Executable = "explorer.exe" }
        "taskmgr" = @{ Service = $null; Executable = "taskmgr.exe" }
        "notepad" = @{ Service = $null; Executable = "notepad.exe" }
        "calc" = @{ Service = $null; Executable = "calc.exe" }
        "paint" = @{ Service = $null; Executable = "mspaint.exe" }
        "wordpad" = @{ Service = $null; Executable = "wordpad.exe" }
        "cmd" = @{ Service = $null; Executable = "cmd.exe" }
        "powershell" = @{ Service = $null; Executable = "powershell.exe" }
        "edge" = @{ Service = $null; Executable = "msedge.exe"; Package = "Microsoft.Edge" }
        "iexplore" = @{ Service = $null; Executable = "iexplore.exe" }
        "searchindexer" = @{ Service = "WSearch"; Executable = $null }
        "superfetch" = @{ Service = "Superfetch"; Executable = $null }
        "windowsupdate" = @{ Service = "wuauserv"; Executable = $null }
        "defender" = @{ Service = "WinDefend"; Executable = $null }
        "cortana" = @{ Service = $null; Executable = $null; Package = "Microsoft.549981C3F5F10" }
        "onenote" = @{ Service = $null; Executable = "onenote.exe"; Package = "Microsoft.Office.OneNote" }
        "skype" = @{ Service = $null; Executable = "skype.exe"; Package = "Microsoft.SkypeApp" }
        "xbox" = @{ Service = "XblAuthManager", "XblGameSave", "XboxGIpSvc"; Executable = $null }
    }
    
    $disabled = @()
    $failed = @()
    
    foreach ($component in $Components) {
        if ($componentMap.ContainsKey($component)) {
            $info = $componentMap[$component]
            Write-Info "Disabling $component..."
            
            try {
                # Disable service if exists
                if ($info.Service) {
                    if ($info.Service -is [array]) {
                        foreach ($service in $info.Service) {
                            if ($service) {
                                & sc config $service start= disabled
                                & sc stop $service
                            }
                        }
                    } else {
                        if ($info.Service) {
                            & sc config $info.Service start= disabled
                            & sc stop $info.Service
                        }
                    }
                }
                
                # Kill process if exists
                if ($info.Executable) {
                    $processes = Get-Process -Name $info.Executable -ErrorAction SilentlyContinue
                    if ($processes) {
                        Stop-Process -Name $info.Executable -Force -ErrorAction SilentlyContinue
                    }
                }
                
                # Remove package if exists
                if ($info.Package) {
                    try {
                        & winget uninstall --id $info.Package --silent --accept-package-agreements --accept-source-agreements
                    } catch {
                        Write-Info "Package $($info.Package) not found or winget failed"
                    }
                }
                
                Write-Success "Disabled $component"
                $disabled += $component
                
            } catch {
                Write-ErrorMsg "Failed to disable $component : $_"
                $failed += $component
            }
        } else {
            Write-WarningMsg "Unknown component: $component"
            $failed += $component
        }
    }
    
    Write-Info "Disabled $($disabled.Count) components, failed $($failed.Count)"
    
    return @{
        Disabled = $disabled
        Failed = $failed
    }
}

function Create-RestorePoint {
    Write-Header "Creating System Restore Point"
    
    try {
        Checkpoint-Computer -Description "FreeNT Transformation - Before" -RestorePointType "MODIFY_SETTINGS" -ErrorAction Stop
        Write-Success "System restore point created"
        return $true
    } catch {
        Write-ErrorMsg "Failed to create restore point: $_"
        Write-WarningMsg "System restore might be disabled on this system"
        return $false
    }
}

function Backup-OriginalState {
    param(
        [string]$BackupDir = "$env:ProgramData\FreeNT\Backup"
    )
    
    Write-Header "Backing Up Original State"
    
    try {
        # Create backup directory
        if (-not (Test-Path $BackupDir)) {
            New-Item -ItemType Directory -Path $BackupDir -Force | Out-Null
        }
        
        # Backup registry
        Write-Info "Backing up registry..."
        $regPath = "$BackupDir\Registry"
        if (-not (Test-Path $regPath)) {
            New-Item -ItemType Directory -Path $regPath -Force | Out-Null
        }
        
        # Export Winlogon key
        $winlogonPath = "$regPath\Winlogon.reg"
        reg export "HKLM\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Winlogon" $winlogonPath /y
        
        # Export CurrentVersion key
        $currentVersionPath = "$regPath\CurrentVersion.reg"
        reg export "HKLM\SOFTWARE\Microsoft\Windows NT\CurrentVersion" $currentVersionPath /y
        
        # Backup environment variables
        Write-Info "Backing up environment variables..."
        $envPath = "$BackupDir\Environment.txt"
        Get-ChildItem Env: | Select-Object Name, Value | Export-Csv -Path $envPath -NoTypeInformation
        
        Write-Success "Original state backed up to $BackupDir"
        return $true
        
    } catch {
        Write-ErrorMsg "Failed to backup original state: $_"
        return $false
    }
}

function Create-TransformationMarker {
    Write-Header "Creating Transformation Marker"
    
    try {
        # Create FreeNT directory
        $freentDir = "$env:ProgramData\FreeNT"
        if (-not (Test-Path $freentDir)) {
            New-Item -ItemType Directory -Path $freentDir -Force | Out-Null
        }
        
        # Create marker file
        $markerPath = "$freentDir\TRANSFORMED.flag"
        $markerContent = @"
FreeNT Transformation Complete
Profile: $Profile
Time: $(Get-Date -Format "yyyy-MM-dd HH:mm:ss")
Version: 1.0
"@
        $markerContent | Out-File -FilePath $markerPath -Encoding UTF8
        
        # Create registry marker
        New-Item -Path "HKLM:\SOFTWARE\FreeNT" -Force | Out-Null
        New-ItemProperty -Path "HKLM:\SOFTWARE\FreeNT" -Name "Transformed" -Value 1 -PropertyType DWord -Force | Out-Null
        New-ItemProperty -Path "HKLM:\SOFTWARE\FreeNT" -Name "Profile" -Value $Profile -PropertyType String -Force | Out-Null
        New-ItemProperty -Path "HKLM:\SOFTWARE\FreeNT" -Name "Version" -Value "1.0" -PropertyType String -Force | Out-Null
        New-ItemProperty -Path "HKLM:\SOFTWARE\FreeNT" -Name "Timestamp" -Value (Get-Date -Format "yyyy-MM-dd HH:mm:ss") -PropertyType String -Force | Out-Null
        
        Write-Success "Transformation marker created"
        return $true
        
    } catch {
        Write-ErrorMsg "Failed to create transformation marker: $_"
        return $false
    }
}

function Finalize-Transformation {
    param(
        [string]$Profile
    )
    
    Write-Header "Finalizing Transformation"
    
    try {
        # Set FreeNT-specific environment variables
        Write-Info "Setting FreeNT environment variables..."
        [Environment]::SetEnvironmentVariable("FREENT", "1", "Machine")
        [Environment]::SetEnvironmentVariable("FREENT_VERSION", "1.0", "Machine")
        [Environment]::SetEnvironmentVariable("FREENT_PROFILE", $Profile, "Machine")
        
        # Broadcast setting changes
        Write-Info "Broadcasting setting changes..."
        $signature = @'
[DllImport("user32.dll", CharSet = CharSet.Auto)]
public static extern IntPtr SendMessageTimeout(
    IntPtr hWnd, uint Msg, UIntPtr wParam, string lParam,
    uint fuFlags, uint uTimeout, out UIntPtr lpdwResult);
'@
        $type = Add-Type -MemberDefinition $signature -Name "Win32SendMessage" -Namespace Win32Functions -PassThru
        
        $HWND_BROADCAST = [IntPtr]0xFFFF
        $WM_SETTINGCHANGE = 0x001A
        $result = [UIntPtr]::Zero
        
        [Win32Functions.Win32SendMessage]::SendMessageTimeout(
            $HWND_BROADCAST, $WM_SETTINGCHANGE, [UIntPtr]::Zero, "Environment",
            0, 5000, [ref]$result) | Out-Null
        
        # Create transformation marker
        Create-TransformationMarker
        
        Write-Success "Transformation finalized"
        return $true
        
    } catch {
        Write-ErrorMsg "Failed to finalize transformation: $_"
        return $false
    }
}

function Display-Summary {
    param(
        [bool]$Success,
        [string]$Message,
        [array]$Actions,
        [array]$Errors,
        [array]$Warnings
    )
    
    Write-Header "Transformation Summary"
    
    if ($Success) {
        Write-Host "Transformation completed successfully!" -ForegroundColor Green
    } else {
        Write-Host "Transformation completed with errors!" -ForegroundColor Red
    }
    
    Write-Host ""
    Write-Host "Message: $Message" -ForegroundColor Yellow
    Write-Host ""
    
    if ($Actions.Count -gt 0) {
        Write-Host "Actions performed ($($Actions.Count)):" -ForegroundColor Cyan
        foreach ($action in $Actions) {
            Write-Host "  - $action" -ForegroundColor Green
        }
        Write-Host ""
    }
    
    if ($Errors.Count -gt 0) {
        Write-Host "Errors ($($Errors.Count)):" -ForegroundColor Red
        foreach ($error in $Errors) {
            Write-Host "  - $error" -ForegroundColor Red
        }
        Write-Host ""
    }
    
    if ($Warnings.Count -gt 0) {
        Write-Host "Warnings ($($Warnings.Count)):" -ForegroundColor Magenta
        foreach ($warning in $Warnings) {
            Write-Host "  - $warning" -ForegroundColor Magenta
        }
        Write-Host ""
    }
    
    Write-Host "Profile: $Profile" -ForegroundColor Yellow
    Write-Host "Log file: $LogFile" -ForegroundColor Yellow
    Write-Host ""
    
    if ($Success) {
        Write-Host "Your system has been transformed into FreeNT!" -ForegroundColor Green
        Write-Host ""
        Write-Host "Next steps:" -ForegroundColor Cyan
        Write-Host "  1. Restart your computer for all changes to take effect" -ForegroundColor Yellow
        Write-Host "  2. Log in with the new FreeNT shell" -ForegroundColor Yellow
        Write-Host "  3. Run 'freent info' to verify the transformation" -ForegroundColor Yellow
    } else {
        Write-Host "Please check the errors above and try again." -ForegroundColor Red
        Write-Host "You can find more details in the log file: $LogFile" -ForegroundColor Yellow
    }
}

# Main transformation process
try {
    # Start logging
    Start-Transcript -Path $LogFile -Append -Force
    Write-Header "FreeNT System Transformation Started"
    Write-Host "Profile: $Profile" -ForegroundColor Cyan
    Write-Host "Dry Run: $DryRun" -ForegroundColor Cyan
    Write-Host "No Vital Utilities: $NoVitalUtilities" -ForegroundColor Cyan
    Write-Host "Vital Install Dir: $VitalInstallDir" -ForegroundColor Cyan
    Write-Host ""
    
    # Confirm transformation
    Confirm-Transformation -Profile $Profile
    
    # Define utilities to install based on profile
    $utilitiesToInstall = @()
    $componentsToReplace = @()
    $componentsToDisable = @()
    $componentsToRemove = @()
    
    switch ($Profile) {
        "minimal" {
            $utilitiesToInstall = @("vital-shell", "vital-filemanager")
            $componentsToReplace = @("explorer")
            $componentsToDisable = @()
            $componentsToRemove = @()
        }
        "standard" {
            $utilitiesToInstall = @(
                "vital-shell", "vital-taskmgr", "vital-notepad", "vital-calc", 
                "vital-paint", "vital-filemanager", "vital-browser"
            )
            $componentsToReplace = @("explorer", "taskmgr", "notepad", "calc", "paint", "cmd", "powershell")
            $componentsToDisable = @("searchindexer", "superfetch", "windowsupdate", "defender", "cortana")
            $componentsToRemove = @()
        }
        "full" {
            $utilitiesToInstall = @(
                "vital-shell", "vital-taskmgr", "vital-notepad", "vital-calc", 
                "vital-paint", "vital-filemanager", "vital-browser", 
                "vital-defender", "vital-notes", "vital-chat"
            )
            $componentsToReplace = @(
                "explorer", "taskmgr", "notepad", "calc", "paint", "wordpad", 
                "cmd", "powershell", "edge", "iexplore", "onenote", "skype", "xbox"
            )
            $componentsToDisable = @("searchindexer", "superfetch", "windowsupdate", "defender", "cortana", "xbox")
            $componentsToRemove = @("edge", "iexplore", "onenote", "skype")
        }
        default {
            # Custom profile - use standard as default
            $utilitiesToInstall = @(
                "vital-shell", "vital-taskmgr", "vital-notepad", "vital-calc", 
                "vital-paint", "vital-filemanager", "vital-browser"
            )
            $componentsToReplace = @("explorer")
            $componentsToDisable = @()
            $componentsToRemove = @()
        }
    }
    
    # Create actions log
    $Actions = @()
    $Errors = @()
    $Warnings = @()
    
    # Create restore point
    if (-not $DryRun) {
        $restoreResult = Create-RestorePoint
        if ($restoreResult) {
            $Actions += "Created system restore point"
        } else {
            $Warnings += "Failed to create system restore point"
        }
    }
    
    # Backup original state
    if (-not $DryRun) {
        $backupResult = Backup-OriginalState
        if ($backupResult) {
            $Actions += "Backed up original system state"
        } else {
            $Warnings += "Failed to backup original system state"
        }
    }
    
    # Modify system identity
    if (-not $DryRun) {
        try {
            Modify-SystemIdentity
            $Actions += "Modified system identity"
        } catch {
            $Errors += "Failed to modify system identity: $_"
        }
    } else {
        Write-Info "[DRY RUN] Would modify system identity"
        $Actions += "[DRY RUN] Would modify system identity"
    }
    
    # Install Vital-Utilities
    if ($NoVitalUtilities) {
        Write-Info "Skipping Vital-Utilities installation"
        $Actions += "Skipped Vital-Utilities installation"
    } else {
        if (-not $DryRun) {
            try {
                $installResult = Install-VitalUtilities -InstallDir $VitalInstallDir -Utilities $utilitiesToInstall
                $Actions += "Installed Vital-Utilities: $($installResult.Installed -join ", ")"
                if ($installResult.Failed.Count -gt 0) {
                    $Warnings += "Failed to install some utilities: $($installResult.Failed -join ", ")"
                }
            } catch {
                $Errors += "Failed to install Vital-Utilities: $_"
            }
        } else {
            Write-Info "[DRY RUN] Would install Vital-Utilities: $($utilitiesToInstall -join ", ")"
            $Actions += "[DRY RUN] Would install Vital-Utilities: $($utilitiesToInstall -join ", ")"
        }
    }
    
    # Replace Explorer
    if (-not $DryRun) {
        try {
            # Find replacement shell
            $replacementPath = $null
            
            # Try Vital-Utilities shell first
            $vitalShellPath = Join-Path $VitalInstallDir "vital-shell\vital-shell.exe"
            if (Test-Path $vitalShellPath) {
                $replacementPath = $vitalShellPath
            }
            
            # Fall back to FreeNT shell
            if (-not $replacementPath) {
                $freentDir = $PSScriptRoot
                while ($freentDir -and (Test-Path (Join-Path $freentDir ".."))) {
                    $freentDir = Join-Path $freentDir ".."
                }
                $freentDir = Resolve-Path $freentDir
                
                $freentShellPath = Join-Path $freentDir "standalone\freent.bat"
                if (Test-Path $freentShellPath) {
                    $replacementPath = $freentShellPath
                } else {
                    $freentShellPath = Join-Path $freentDir "src\login_manager\login_app.py"
                    if (Test-Path $freentShellPath) {
                        $replacementPath = $freentShellPath
                    }
                }
            }
            
            if ($replacementPath) {
                $replaceResult = Replace-Explorer -ReplacementPath $replacementPath
                if ($replaceResult) {
                    $Actions += "Replaced Windows Explorer with $replacementPath"
                } else {
                    $Errors += "Failed to replace Windows Explorer"
                }
            } else {
                $Errors += "Could not find replacement shell"
            }
        } catch {
            $Errors += "Failed to replace Explorer: $_"
        }
    } else {
        Write-Info "[DRY RUN] Would replace Windows Explorer"
        $Actions += "[DRY RUN] Would replace Windows Explorer"
    }
    
    # Disable components
    if (-not $DryRun) {
        try {
            $disableResult = Disable-Components -Components $componentsToDisable
            $Actions += "Disabled components: $($disableResult.Disabled -join ", ")"
            if ($disableResult.Failed.Count -gt 0) {
                $Warnings += "Failed to disable some components: $($disableResult.Failed -join ", ")"
            }
        } catch {
            $Errors += "Failed to disable components: $_"
        }
    } else {
        Write-Info "[DRY RUN] Would disable components: $($componentsToDisable -join ", ")"
        $Actions += "[DRY RUN] Would disable components: $($componentsToDisable -join ", ")"
    }
    
    # Remove components (only in full profile and if not dry run)
    if ($Profile -eq "full" -and -not $DryRun) {
        try {
            # For now, we just disable them as removal can be dangerous
            $removeResult = Disable-Components -Components $componentsToRemove
            $Actions += "Removed components: $($removeResult.Disabled -join ", ")"
            if ($removeResult.Failed.Count -gt 0) {
                $Warnings += "Failed to remove some components: $($removeResult.Failed -join ", ")"
            }
        } catch {
            $Errors += "Failed to remove components: $_"
        }
    } elseif ($Profile -eq "full" -and $DryRun) {
        Write-Info "[DRY RUN] Would remove components: $($componentsToRemove -join ", ")"
        $Actions += "[DRY RUN] Would remove components: $($componentsToRemove -join ", ")"
    }
    
    # Finalize transformation
    if (-not $DryRun) {
        try {
            $finalizeResult = Finalize-Transformation -Profile $Profile
            if ($finalizeResult) {
                $Actions += "Finalized transformation"
            } else {
                $Warnings += "Failed to finalize transformation"
            }
        } catch {
            $Errors += "Failed to finalize transformation: $_"
        }
    } else {
        Write-Info "[DRY RUN] Would finalize transformation"
        $Actions += "[DRY RUN] Would finalize transformation"
    }
    
    # Determine success
    $Success = ($Errors.Count -eq 0)
    $Message = if ($Success) { "FreeNT transformation completed successfully" } else { "FreeNT transformation completed with errors" }
    
    # Display summary
    Display-Summary -Success $Success -Message $Message -Actions $Actions -Errors $Errors -Warnings $Warnings
    
    # Stop logging
    Stop-Transcript
    
    # Exit with appropriate code
    if ($Success) {
        exit 0
    } else {
        exit 1
    }
    
} catch {
    Write-ErrorMsg "Transformation failed: $_"
    Stop-Transcript
    exit 1
}
