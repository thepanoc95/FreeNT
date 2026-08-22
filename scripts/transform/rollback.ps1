# FreeNT Transformation Rollback Script
# Copyright (c) 2026, Panoc95
# BSD 3-Clause License

<#
.SYNOPSIS
    Rolls back FreeNT transformation to restore original Windows state.

.DESCRIPTION
    This script restores the system to its original state before FreeNT
    transformation was applied. It performs the following actions:
    - Restores Windows Explorer
    - Restores system identity
    - Re-enables disabled services
    - Removes Vital-Utilities (optional)
    - Restores original registry settings

.NOTES
    File Name      : rollback.ps1
    Author         : Panoc95
    Prerequisite   : PowerShell 5.1 or later
    Run as Administrator: REQUIRED
#>

param(
    [switch]$Force,
    [switch]$SkipConfirm,
    [switch]$RemoveVitalUtilities,
    [string]$BackupDir = "$env:ProgramData\FreeNT\Backup",
    [string]$LogFile = "$env:TEMP\FreeNT_Rollback.log"
)

# Require admin privileges
if (-NOT ([Security.Principal.WindowsPrincipal] [Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole] "Administrator")) {
    Write-Warning "This script MUST be run as Administrator!"
    Write-Host "Rollback requires elevated privileges to restore system components."
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

function Confirm-Rollback {
    Write-Header "FreeNT Transformation Rollback"
    Write-Host "This script will restore your Windows system to its original state." -ForegroundColor Yellow
    Write-Host ""
    Write-Host "This rollback will:" -ForegroundColor Yellow
    Write-Host "  - Restore Windows Explorer" -ForegroundColor Yellow
    Write-Host "  - Restore system identity" -ForegroundColor Yellow
    Write-Host "  - Re-enable disabled services" -ForegroundColor Yellow
    Write-Host "  - Restore original registry settings" -ForegroundColor Yellow
    if ($RemoveVitalUtilities) {
        Write-Host "  - Remove Vital-Utilities" -ForegroundColor Yellow
    }
    Write-Host ""
    Write-Host "WARNING: This will revert all FreeNT changes." -ForegroundColor Red
    Write-Host "Your system will be restored to its original Windows state." -ForegroundColor Yellow
    Write-Host ""
    
    if (-not $SkipConfirm) {
        $response = Read-Host "Do you want to proceed? (Y/N)"
        if ($response -notmatch "^[yY]") {
            Write-Info "Rollback cancelled by user"
            exit 0
        }
    }
}

function Restore-Explorer {
    Write-Header "Restoring Windows Explorer"
    
    try {
        # Check if FreeNT transformation marker exists
        $freentDir = "$env:ProgramData\FreeNT"
        $markerPath = "$freentDir\TRANSFORMED.flag"
        
        if (-not (Test-Path $markerPath)) {
            Write-Info "No FreeNT transformation marker found"
            return $false
        }
        
        # Read original shell from backup
        $backupDir = "$env:ProgramData\FreeNT\Backup"
        $winlogonPath = "$backupDir\Registry\Winlogon.reg"
        
        if (Test-Path $winlogonPath) {
            Write-Info "Restoring original shell from backup..."
            
            # Parse the reg file to get original shell
            $regContent = Get-Content $winlogonPath -Raw
            $shellMatch = [regex]::Match($regContent, '"Shell"="([^"]+)"')
            
            if ($shellMatch.Success) {
                $originalShell = $shellMatch.Groups[1].Value
                Write-Info "Original shell: $originalShell"
                
                # Restore registry
                $regPath = "HKLM:\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Winlogon"
                Set-ItemProperty -Path $regPath -Name "Shell" -Value $originalShell -Force
                
                # Restore Userinit if backed up
                $userinitMatch = [regex]::Match($regContent, '"Userinit"="([^"]+)"')
                if ($userinitMatch.Success) {
                    $originalUserinit = $userinitMatch.Groups[1].Value
                    Set-ItemProperty -Path $regPath -Name "Userinit" -Value $originalUserinit -Force
                } else {
                    # Remove Userinit if it exists (to restore default)
                    Remove-ItemProperty -Path $regPath -Name "Userinit" -ErrorAction SilentlyContinue
                }
                
                Write-Success "Registry restored from backup"
            } else {
                # Use default values
                Write-Info "Using default Windows Explorer settings"
                $regPath = "HKLM:\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Winlogon"
                Set-ItemProperty -Path $regPath -Name "Shell" -Value "explorer.exe" -Force
                Remove-ItemProperty -Path $regPath -Name "Userinit" -ErrorAction SilentlyContinue
                Write-Success "Registry restored with defaults"
            }
        } else {
            # Use default values
            Write-Info "Using default Windows Explorer settings"
            $regPath = "HKLM:\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Winlogon"
            Set-ItemProperty -Path $regPath -Name "Shell" -Value "explorer.exe" -Force
            Remove-ItemProperty -Path $regPath -Name "Userinit" -ErrorAction SilentlyContinue
            Write-Success "Registry restored with defaults"
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
        
        # Start Explorer
        Write-Info "Starting Windows Explorer..."
        Start-Process explorer.exe
        
        Write-Success "Windows Explorer restored successfully"
        return $true
        
    } catch {
        Write-ErrorMsg "Failed to restore Explorer: $_"
        return $false
    }
}

function Restore-SystemIdentity {
    Write-Header "Restoring System Identity"
    
    try {
        # Restore registry branding
        Write-Info "Restoring registry branding..."
        
        $regPaths = @(
            "HKLM:\SOFTWARE\Microsoft\Windows NT\CurrentVersion",
            "HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion"
        )
        
        foreach ($regPath in $regPaths) {
            try {
                # Remove FreeNT-specific values
                Remove-ItemProperty -Path $regPath -Name "ProductName" -ErrorAction SilentlyContinue
                Remove-ItemProperty -Path $regPath -Name "DisplayVersion" -ErrorAction SilentlyContinue
                Remove-ItemProperty -Path $regPath -Name "CurrentBuild" -ErrorAction SilentlyContinue
                Remove-ItemProperty -Path $regPath -Name "CurrentVersion" -ErrorAction SilentlyContinue
                Remove-ItemProperty -Path $regPath -Name "RegisteredOrganization" -ErrorAction SilentlyContinue
                Remove-ItemProperty -Path $regPath -Name "RegisteredOwner" -ErrorAction SilentlyContinue
                
                Write-Success "Cleaned $regPath"
                
            } catch {
                Write-ErrorMsg "Failed to clean $regPath : $_"
            }
        }
        
        # Remove FreeNT registry key
        try {
            Remove-Item -Path "HKLM:\SOFTWARE\FreeNT" -Recurse -Force -ErrorAction SilentlyContinue
            Write-Success "Removed FreeNT registry key"
        } catch {
            Write-Info "FreeNT registry key not found"
        }
        
        # Remove environment variables
        Write-Info "Removing FreeNT environment variables..."
        [Environment]::SetEnvironmentVariable("FREENT", $null, "Machine")
        [Environment]::SetEnvironmentVariable("FREENT_VERSION", $null, "Machine")
        [Environment]::SetEnvironmentVariable("FREENT_PROFILE", $null, "Machine")
        [Environment]::SetEnvironmentVariable("FREENT_FULL_TRANSFORM", $null, "Machine")
        Write-Success "Removed FreeNT environment variables"
        
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
        
        Write-Success "System identity restored successfully"
        return $true
        
    } catch {
        Write-ErrorMsg "Failed to restore system identity: $_"
        return $false
    }
}

function Enable-Components {
    Write-Header "Enabling Disabled Components"
    
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
    
    # List of services that were likely disabled
    $servicesToEnable = @(
        "ShellHWDetection", "WSearch", "Superfetch", "wuauserv", "WinDefend",
        "XblAuthManager", "XblGameSave", "XboxGIpSvc"
    )
    
    $enabled = @()
    $failed = @()
    
    foreach ($service in $servicesToEnable) {
        Write-Info "Enabling service: $service..."
        try {
            & sc config $service start= auto
            & sc start $service
            Write-Success "Enabled $service"
            $enabled += $service
        } catch {
            Write-ErrorMsg "Failed to enable $service : $_"
            $failed += $service
        }
    }
    
    Write-Info "Enabled $($enabled.Count) services, failed $($failed.Count)"
    
    return @{
        Enabled = $enabled
        Failed = $failed
    }
}

function Remove-VitalUtilities {
    param(
        [string]$InstallDir = "$env:ProgramFiles\Vital-Utilities"
    )
    
    Write-Header "Removing Vital-Utilities"
    
    try {
        if (-not (Test-Path $InstallDir)) {
            Write-Info "Vital-Utilities not found at $InstallDir"
            return $false
        }
        
        Write-Info "Removing Vital-Utilities from $InstallDir..."
        Remove-Item -Path $InstallDir -Recurse -Force
        
        Write-Success "Vital-Utilities removed successfully"
        return $true
        
    } catch {
        Write-ErrorMsg "Failed to remove Vital-Utilities: $_"
        return $false
    }
}

function Remove-TransformationMarker {
    Write-Header "Removing Transformation Marker"
    
    try {
        # Remove marker file
        $markerPath = "$env:ProgramData\FreeNT\TRANSFORMED.flag"
        if (Test-Path $markerPath) {
            Remove-Item -Path $markerPath -Force
            Write-Success "Removed transformation marker file"
        }
        
        # Remove registry marker
        try {
            Remove-Item -Path "HKLM:\SOFTWARE\FreeNT" -Recurse -Force -ErrorAction SilentlyContinue
            Write-Success "Removed FreeNT registry key"
        } catch {
            Write-Info "FreeNT registry key not found"
        }
        
        return $true
        
    } catch {
        Write-ErrorMsg "Failed to remove transformation marker: $_"
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
    
    Write-Header "Rollback Summary"
    
    if ($Success) {
        Write-Host "Rollback completed successfully!" -ForegroundColor Green
    } else {
        Write-Host "Rollback completed with errors!" -ForegroundColor Red
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
    
    Write-Host "Log file: $LogFile" -ForegroundColor Yellow
    Write-Host ""
    
    if ($Success) {
        Write-Host "Your system has been restored to its original Windows state!" -ForegroundColor Green
        Write-Host ""
        Write-Host "Next steps:" -ForegroundColor Cyan
        Write-Host "  1. Restart your computer for all changes to take effect" -ForegroundColor Yellow
        Write-Host "  2. Windows Explorer should be restored" -ForegroundColor Yellow
    } else {
        Write-Host "Please check the errors above and try again." -ForegroundColor Red
        Write-Host "You can find more details in the log file: $LogFile" -ForegroundColor Yellow
    }
}

# Main rollback process
try {
    # Start logging
    Start-Transcript -Path $LogFile -Append -Force
    Write-Header "FreeNT Transformation Rollback Started"
    Write-Host "Remove Vital-Utilities: $RemoveVitalUtilities" -ForegroundColor Cyan
    Write-Host "Backup Directory: $BackupDir" -ForegroundColor Cyan
    Write-Host ""
    
    # Confirm rollback
    Confirm-Rollback
    
    # Create actions log
    $Actions = @()
    $Errors = @()
    $Warnings = @()
    
    # Check if transformation was applied
    $freentDir = "$env:ProgramData\FreeNT"
    $markerPath = "$freentDir\TRANSFORMED.flag"
    
    if (-not (Test-Path $markerPath)) {
        Write-Info "No FreeNT transformation marker found"
        Write-Host "This system does not appear to have been transformed." -ForegroundColor Yellow
        
        $response = Read-Host "Do you want to continue anyway? (Y/N)"
        if ($response -notmatch "^[yY]") {
            Write-Info "Rollback cancelled by user"
            Stop-Transcript
            exit 0
        }
    }
    
    # Restore system identity
    try {
        $identityResult = Restore-SystemIdentity
        if ($identityResult) {
            $Actions += "Restored system identity"
        } else {
            $Errors += "Failed to restore system identity"
        }
    } catch {
        $Errors += "Failed to restore system identity: $_"
    }
    
    # Restore Explorer
    try {
        $explorerResult = Restore-Explorer
        if ($explorerResult) {
            $Actions += "Restored Windows Explorer"
        } else {
            $Errors += "Failed to restore Windows Explorer"
        }
    } catch {
        $Errors += "Failed to restore Explorer: $_"
    }
    
    # Enable components
    try {
        $enableResult = Enable-Components
        $Actions += "Enabled services: $($enableResult.Enabled -join ", ")"
        if ($enableResult.Failed.Count -gt 0) {
            $Warnings += "Failed to enable some services: $($enableResult.Failed -join ", ")"
        }
    } catch {
        $Errors += "Failed to enable components: $_"
    }
    
    # Remove Vital-Utilities if requested
    if ($RemoveVitalUtilities) {
        try {
            $removeResult = Remove-VitalUtilities
            if ($removeResult) {
                $Actions += "Removed Vital-Utilities"
            } else {
                $Warnings += "Vital-Utilities not found or already removed"
            }
        } catch {
            $Errors += "Failed to remove Vital-Utilities: $_"
        }
    }
    
    # Remove transformation marker
    try {
        $markerResult = Remove-TransformationMarker
        if ($markerResult) {
            $Actions += "Removed transformation marker"
        } else {
            $Warnings += "Failed to remove transformation marker"
        }
    } catch {
        $Errors += "Failed to remove transformation marker: $_"
    }
    
    # Determine success
    $Success = ($Errors.Count -eq 0)
    $Message = if ($Success) { "FreeNT rollback completed successfully" } else { "FreeNT rollback completed with errors" }
    
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
    Write-ErrorMsg "Rollback failed: $_"
    Stop-Transcript
    exit 1
}
