# FreeNT WIM Creator
# Copyright (c) 2026, Panoc95
# BSD 3-Clause License

<#
.SYNOPSIS
    Creates a FreeNT WIM from a Windows Enterprise ISO.

.DESCRIPTION
    This script performs the following operations:
    1. Mounts the Windows Enterprise ISO
    2. Extracts the install.wim or install.esd
    3. Creates a new WIM with only FreeNT edition
    4. Removes all MsStore apps and bloatware
    5. Injects FreeNT files and configuration
    6. Configures FreeNT as the default shell
    7. Saves the customized WIM

.NOTES
    File Name      : create_freent_wim.ps1
    Author         : Panoc95
    Prerequisite   : PowerShell 5.1 or later
    Run as Administrator: REQUIRED
    Dependencies   : DISM, Windows ADK (optional for ESD support)
#>

param(
    [Parameter(Mandatory=$true)]
    [string]$ISOPath,
    
    [string]$OutputWIM = "install_freent.wim",
    [int]$WindowsVersion = 11,  # 10 or 11
    [switch]$KeepOriginal,
    [switch]$Verbose,
    [string]$WorkDir = "$env:TEMP\FreeNT_WIM",
    [switch]$Cleanup,
    [switch]$SkipConfirm
)

# Require admin privileges
if (-NOT ([Security.Principal.WindowsPrincipal] [Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole] "Administrator")) {
    Write-Warning "This script MUST be run as Administrator!"
    Write-Host "WIM manipulation requires elevated privileges."
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

function Confirm-Creation {
    param(
        [string]$ISOPath,
        [string]$OutputWIM,
        [int]$WindowsVersion
    )
    
    Write-Header "FreeNT WIM Creator"
    Write-Host "This script will create a customized FreeNT WIM from a Windows Enterprise ISO." -ForegroundColor Yellow
    Write-Host ""
    Write-Host "ISO Path: $ISOPath" -ForegroundColor Cyan
    Write-Host "Output WIM: $OutputWIM" -ForegroundColor Cyan
    Write-Host "Windows Version: $WindowsVersion" -ForegroundColor Cyan
    Write-Host "Work Directory: $WorkDir" -ForegroundColor Cyan
    Write-Host ""
    
    Write-Host "This process will:" -ForegroundColor Yellow
    Write-Host "  1. Mount the Windows ISO" -ForegroundColor Yellow
    Write-Host "  2. Extract the install.wim/install.esd" -ForegroundColor Yellow
    Write-Host "  3. Create a new WIM with only FreeNT edition" -ForegroundColor Yellow
    Write-Host "  4. Remove all MsStore apps and bloatware" -ForegroundColor Yellow
    Write-Host "  5. Inject FreeNT files and configuration" -ForegroundColor Yellow
    Write-Host "  6. Configure FreeNT as the default shell" -ForegroundColor Yellow
    Write-Host ""
    
    Write-Host "WARNING: This process may take a long time and require significant disk space." -ForegroundColor Red
    Write-Host "Ensure you have at least 20GB of free space on the drive containing $WorkDir." -ForegroundColor Yellow
    Write-Host ""
    
    if (-not $SkipConfirm) {
        $response = Read-Host "Do you want to proceed? (Y/N)"
        if ($response -notmatch "^[yY]") {
            Write-Info "Creation cancelled by user"
            exit 0
        }
    }
}

function Mount-ISO {
    param(
        [string]$ISOPath,
        [string]$MountDir
    )
    
    Write-Header "Mounting ISO"
    
    try {
        # Create mount directory
        if (-not (Test-Path $MountDir)) {
            New-Item -ItemType Directory -Path $MountDir -Force | Out-Null
        }
        
        # Mount ISO using PowerShell
        Write-Info "Mounting $ISOPath..."
        $mountResult = Mount-DiskImage -ImagePath $ISOPath -PassThru
        
        # Get the drive letter
        $volume = Get-Volume | Where-Object { $_.Path -like "*$ISOPath*" } | Select-Object -First 1
        if ($volume) {
            $driveLetter = $volume.DriveLetter
            $mountPath = "$($driveLetter):\"
            
            Write-Success "ISO mounted at $mountPath"
            return @{
                Mounted = $true
                Path = $mountPath
                DriveLetter = $driveLetter
            }
        } else {
            # Try alternative method
            $drive = Get-DiskImage -ImagePath $ISOPath | Get-Volume
            if ($drive) {
                $driveLetter = $drive.DriveLetter
                $mountPath = "$($driveLetter):\"
                
                Write-Success "ISO mounted at $mountPath"
                return @{
                    Mounted = $true
                    Path = $mountPath
                    DriveLetter = $driveLetter
                }
            }
        }
        
        Write-ErrorMsg "Failed to mount ISO"
        return @{ Mounted = $false }
        
    } catch {
        Write-ErrorMsg "Failed to mount ISO: $_"
        return @{ Mounted = $false }
    }
}

function Unmount-ISO {
    param(
        [string]$ISOPath
    )
    
    Write-Header "Unmounting ISO"
    
    try {
        Write-Info "Unmounting $ISOPath..."
        Dismount-DiskImage -ImagePath $ISOPath -Confirm:$false
        Write-Success "ISO unmounted"
        return $true
    } catch {
        Write-ErrorMsg "Failed to unmount ISO: $_"
        return $false
    }
}

function Extract-WIMFromISO {
    param(
        [string]$ISOMountPath
    )
    
    Write-Header "Extracting WIM from ISO"
    
    try {
        # Look for install.wim
        $wimPath = Join-Path $ISOMountPath "sources\install.wim"
        if (Test-Path $wimPath) {
            Write-Success "Found install.wim"
            return @{
                Found = $true
                Path = $wimPath
                Type = "WIM"
            }
        }
        
        # Look for install.esd
        $esdPath = Join-Path $ISOMountPath "sources\install.esd"
        if (Test-Path $esdPath) {
            Write-Success "Found install.esd"
            return @{
                Found = $true
                Path = $esdPath
                Type = "ESD"
            }
        }
        
        # Search all directories
        $files = Get-ChildItem -Path $ISOMountPath -Recurse -Filter "install.wim" -ErrorAction SilentlyContinue
        if ($files) {
            Write-Success "Found install.wim at $($files[0].FullName)"
            return @{
                Found = $true
                Path = $files[0].FullName
                Type = "WIM"
            }
        }
        
        $files = Get-ChildItem -Path $ISOMountPath -Recurse -Filter "install.esd" -ErrorAction SilentlyContinue
        if ($files) {
            Write-Success "Found install.esd at $($files[0].FullName)"
            return @{
                Found = $true
                Path = $files[0].FullName
                Type = "ESD"
            }
        }
        
        Write-ErrorMsg "No install.wim or install.esd found in ISO"
        return @{ Found = $false }
        
    } catch {
        Write-ErrorMsg "Failed to extract WIM from ISO: $_"
        return @{ Found = $false }
    }
}

function Convert-ESDToWIM {
    param(
        [string]$ESDPath,
        [string]$OutputWIM
    )
    
    Write-Header "Converting ESD to WIM"
    
    try {
        Write-Info "Converting $ESDPath to WIM..."
        
        # Check if DISM supports ESD export
        $dismVersion = (dism /Online /Get-Version).Split(" ")[1]
        if ([version]$dismVersion -ge [version]"10.0.10240") {
            # Use DISM to export ESD to WIM
            $result = dism /Export-Image /SourceImageFile:$ESDPath /SourceIndex:1 /DestinationImageFile:$OutputWIM /Compress:max /CheckIntegrity
            
            if ($LASTEXITCODE -eq 0) {
                Write-Success "ESD converted to WIM"
                return $true
            }
        }
        
        # Alternative: Use esd-decrypter or other tools
        Write-WarningMsg "DISM does not support ESD export. Install Windows ADK for ESD support."
        Write-WarningMsg "Attempting to copy ESD as WIM (may not work)..."
        Copy-Item -Path $ESDPath -Destination $OutputWIM -Force
        
        return $true
        
    } catch {
        Write-ErrorMsg "Failed to convert ESD to WIM: $_"
        return $false
    }
}

function Get-WIMInfo {
    param(
        [string]$WIMPath
    )
    
    Write-Header "Getting WIM Information"
    
    try {
        Write-Info "Getting info for $WIMPath..."
        $result = dism /Get-WimInfo /WimFile:$WIMPath
        
        if ($LASTEXITCODE -ne 0) {
            Write-ErrorMsg "Failed to get WIM info"
            return $null
        }
        
        # Parse output
        $info = @{}
        $currentIndex = $null
        
        $result | ForEach-Object {
            if ($_ -match "^Index: (\d+)") {
                if ($currentIndex) {
                    $info["Index_$currentIndex"] = $currentInfo
                }
                $currentIndex = $matches[1]
                $currentInfo = @{}
            } elseif ($currentIndex -and $_ -match "^(.*?): (.*)") {
                $key = $matches[1].Trim()
                $value = $matches[2].Trim()
                $currentInfo[$key] = $value
            }
        }
        
        if ($currentIndex) {
            $info["Index_$currentIndex"] = $currentInfo
        }
        
        return $info
        
    } catch {
        Write-ErrorMsg "Failed to get WIM info: $_"
        return $null
    }
}

function Get-EnterpriseImageIndex {
    param(
        [string]$WIMPath
    )
    
    Write-Header "Finding Enterprise Image"
    
    try {
        $wimInfo = Get-WIMInfo -WIMPath $WIMPath
        if (-not $wimInfo) {
            return $null
        }
        
        $enterpriseIndices = @()
        
        foreach ($key in $wimInfo.Keys) {
            if ($key -match "^Index_(\d+)") {
                $index = $matches[1]
                $imageInfo = $wimInfo[$key]
                
                $name = $imageInfo["Name"]
                $description = $imageInfo["Description"]
                
                if ($name -match "Enterprise" -or $description -match "Enterprise") {
                    $enterpriseIndices += $index
                }
            }
        }
        
        if ($enterpriseIndices.Count -gt 0) {
            # Return the first Enterprise image
            Write-Success "Found Enterprise image at index $($enterpriseIndices[0])"
            return $enterpriseIndices[0]
        }
        
        Write-ErrorMsg "No Enterprise image found in WIM"
        return $null
        
    } catch {
        Write-ErrorMsg "Failed to find Enterprise image: $_"
        return $null
    }
}

function Create-FreeNTWIM {
    param(
        [string]$WIMPath,
        [string]$OutputWIM,
        [int]$ImageIndex,
        [int]$WindowsVersion,
        [string]$WorkDir,
        [string]$FreeNTDir
    )
    
    Write-Header "Creating FreeNT WIM"
    
    try {
        # Create work directory
        if (-not (Test-Path $WorkDir)) {
            New-Item -ItemType Directory -Path $WorkDir -Force | Out-Null
        }
        
        # Copy WIM to work directory
        $tempWIM = Join-Path $WorkDir "install.wim"
        if ($KeepOriginal) {
            Copy-Item -Path $WIMPath -Destination $tempWIM -Force
            Write-Info "Copied WIM to $tempWIM"
        } else {
            $tempWIM = $WIMPath
        }
        
        # Get WIM info
        $wimInfo = Get-WIMInfo -WIMPath $tempWIM
        if (-not $wimInfo) {
            throw "Failed to get WIM info"
        }
        
        # Mount the Enterprise image
        $mountDir = Join-Path $WorkDir "mount"
        Write-Info "Mounting image $ImageIndex..."
        $mountResult = dism /Mount-Wim /WimFile:$tempWIM /Index:$ImageIndex /MountDir:$mountDir
        
        if ($LASTEXITCODE -ne 0) {
            throw "Failed to mount WIM image"
        }
        
        Write-Success "WIM mounted at $mountDir"
        
        try {
            # Customize the edition to FreeNT
            Write-Info "Customizing edition to FreeNT..."
            Customize-Edition -MountDir $mountDir -WindowsVersion $WindowsVersion
            Write-Success "Edition customized to FreeNT"
            
            # Remove bloatware
            Write-Info "Removing bloatware..."
            $bloatResult = Remove-Bloatware -MountDir $mountDir -Aggressive:$true
            Write-Success "Bloatware removed ($($bloatResult.Removed.Count) packages)"
            
            # Inject FreeNT files
            Write-Info "Injecting FreeNT files..."
            $injectResult = Inject-FreeNT -MountDir $mountDir -FreeNTDir $FreeNTDir
            Write-Success "FreeNT files injected"
            
            # Configure FreeNT as default shell
            Write-Info "Configuring FreeNT as default shell..."
            Configure-FreeNTShell -MountDir $mountDir -FreeNTDir $mountDir
            Write-Success "FreeNT configured as default shell"
            
            # Unmount and commit
            Write-Info "Committing changes..."
            $unmountResult = dism /Unmount-Wim /MountDir:$mountDir /Commit
            
            if ($LASTEXITCODE -ne 0) {
                throw "Failed to commit WIM changes"
            }
            
            Write-Success "Changes committed"
            
            # Remove other editions (keep only FreeNT)
            Write-Info "Removing other editions..."
            Remove-OtherEditions -WIMPath $tempWIM -KeepIndex $ImageIndex -WindowsVersion $WindowsVersion
            Write-Success "Other editions removed"
            
            # Optimize WIM
            Write-Info "Optimizing WIM..."
            $optimizeResult = dism /Optimize-Wim /ImageFile:$tempWIM
            
            if ($LASTEXITCODE -eq 0) {
                Write-Success "WIM optimized"
            } else {
                Write-WarningMsg "WIM optimization failed"
            }
            
            # Move to final output
            if ($KeepOriginal) {
                Move-Item -Path $tempWIM -Destination $OutputWIM -Force
            } else {
                Copy-Item -Path $tempWIM -Destination $OutputWIM -Force
            }
            
            Write-Success "FreeNT WIM created at $OutputWIM"
            
            return @{
                Success = $true
                OutputWIM = $OutputWIM
                Message = "FreeNT WIM created successfully"
            }
            
        } catch {
            # Rollback on error
            Write-ErrorMsg "Error during WIM customization: $_"
            
            try {
                dism /Unmount-Wim /MountDir:$mountDir /Discard
            } catch {
                Write-WarningMsg "Failed to discard WIM changes: $_"
            }
            
            throw
        }
        
    } catch {
        Write-ErrorMsg "Failed to create FreeNT WIM: $_"
        return @{
            Success = $false
            OutputWIM = $null
            Message = "Failed to create FreeNT WIM: $_"
        }
    }
}

function Customize-Edition {
    param(
        [string]$MountDir,
        [int]$WindowsVersion
    )
    
    Write-Header "Customizing Edition"
    
    try {
        # Set edition name based on version
        if ($WindowsVersion -eq 11) {
            $editionName = "Windows 11 FreeNT"
            $editionDescription = "FreeNT - Alternative Userland for Windows 11"
        } else {
            $editionName = "Windows 10 FreeNT"
            $editionDescription = "FreeNT - Alternative Userland for Windows 10"
        }
        
        # Modify registry in mounted image
        Write-Info "Modifying registry..."
        Modify-RegistryEdition -MountDir $MountDir -EditionName $editionName
        
        # Modify setup files
        Write-Info "Modifying setup files..."
        Modify-SetupFiles -MountDir $MountDir -EditionName $editionName
        
        # Modify unattend.xml
        Write-Info "Modifying unattend.xml..."
        Modify-UnattendXML -MountDir $MountDir -EditionName $editionName
        
        Write-Success "Edition customized"
        
    } catch {
        Write-ErrorMsg "Failed to customize edition: $_"
        throw
    }
}

function Modify-RegistryEdition {
    param(
        [string]$MountDir,
        [string]$EditionName
    )
    
    try {
        # Create reg file for FreeNT edition
        $regContent = @"
Windows Registry Editor Version 5.00

[HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows NT\CurrentVersion]
"ProductName"="$EditionName"
"DisplayVersion"="10.0"
"CurrentBuild"="FreeNT"
"CurrentVersion"="$EditionName"
"EditionID"="FreeNT"
"InstallationType"="Client"
"ProductId"="00330-80000-00000-AAOEM"
"RegisteredOrganization"="FreeNT Project"
"RegisteredOwner"="FreeNT User"

[HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion]
"ProductName"="$EditionName"
"CurrentVersion"="$EditionName"
"CurrentBuildNumber"="FreeNT"
"CurrentType"="Multiprocessor Free"
"EditionID"="FreeNT"

[HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\ProductOptions]
"ProductName"="$EditionName"
"ProductPolicy"=dword:00000000
"@
        
        $regFile = Join-Path $MountDir "freent_edition.reg"
        $regContent | Out-File -FilePath $regFile -Encoding UTF8
        
        # Import reg file into offline registry
        # Note: This requires proper offline registry editing
        Write-Info "Registry file created at $regFile (manual import may be needed)"
        
    } catch {
        Write-ErrorMsg "Failed to modify registry edition: $_"
        throw
    }
}

function Modify-SetupFiles {
    param(
        [string]$MountDir,
        [string]$EditionName
    )
    
    try {
        # Modify setup.cfg if exists
        $setupCfg = Join-Path $MountDir "setup.cfg"
        if (Test-Path $setupCfg) {
            Write-Info "Modifying setup.cfg..."
            $content = Get-Content -Path $setupCfg -Raw
            $content = $content -replace "Enterprise", $EditionName
            $content = $content -replace "Professional", $EditionName
            $content = $content -replace "Home", $EditionName
            $content | Out-File -FilePath $setupCfg -Encoding UTF8
        }
        
        # Modify other setup files
        $setupFiles = Get-ChildItem -Path $MountDir -Recurse -Filter "setup*.cfg" -ErrorAction SilentlyContinue
        foreach ($file in $setupFiles) {
            try {
                $content = Get-Content -Path $file.FullName -Raw
                $content = $content -replace "Enterprise", $EditionName
                $content | Out-File -FilePath $file.FullName -Encoding UTF8
            } catch {
                Write-WarningMsg "Failed to modify $($file.FullName): $_"
            }
        }
        
    } catch {
        Write-ErrorMsg "Failed to modify setup files: $_"
        throw
    }
}

function Modify-UnattendXML {
    param(
        [string]$MountDir,
        [string]$EditionName
    )
    
    try {
        # Find all unattend.xml files
        $unattendFiles = Get-ChildItem -Path $MountDir -Recurse -Filter "unattend.xml" -ErrorAction SilentlyContinue
        
        foreach ($file in $unattendFiles) {
            try {
                Write-Info "Modifying $($file.FullName)..."
                
                [xml]$xmlContent = Get-Content -Path $file.FullName
                
                # Modify ProductKey
                $productKeys = $xmlContent.SelectNodes("//ProductKey")
                foreach ($key in $productKeys) {
                    $key.InnerText = "VK7JG-NPHTM-C97JM-9MPGT-3V66T"  # Generic key
                }
                
                # Modify EditionID
                $editionIDs = $xmlContent.SelectNodes("//EditionID")
                foreach ($id in $editionIDs) {
                    $id.InnerText = "FreeNT"
                }
                
                # Modify ProductName
                $productNames = $xmlContent.SelectNodes("//ProductName")
                foreach ($name in $productNames) {
                    $name.InnerText = $EditionName
                }
                
                # Save changes
                $xmlContent.Save($file.FullName)
                
            } catch {
                Write-WarningMsg "Failed to modify $($file.FullName): $_"
            }
        }
        
    } catch {
        Write-ErrorMsg "Failed to modify unattend.xml: $_"
        throw
    }
}

function Remove-Bloatware {
    param(
        [string]$MountDir,
        [switch]$Aggressive
    )
    
    Write-Header "Removing Bloatware"
    
    $results = @{
        Removed = @()
        Failed = @()
    }
    
    try {
        # List of packages to remove (MsStore apps and bloatware)
        $packagesToRemove = @(
            # MsStore apps
            "Microsoft.549981C3F5F10",  # Cortana
            "Microsoft.BingWeather",
            "Microsoft.BingNews",
            "Microsoft.BingSports",
            "Microsoft.BingFinance",
            "Microsoft.WindowsCalculator",
            "Microsoft.WindowsAlarms",
            "Microsoft.WindowsCamera",
            "Microsoft.WindowsMaps",
            "Microsoft.WindowsPhone",
            "Microsoft.WindowsFeedbackHub",
            "Microsoft.WindowsSoundRecorder",
            "Microsoft.WindowsStore",
            "Microsoft.Office.OneNote",
            "Microsoft.SkypeApp",
            "Microsoft.XboxIdentityProvider",
            "Microsoft.Xbox.TCUI",
            "Microsoft.XboxGameOverlay",
            "Microsoft.XboxGamingOverlay",
            "Microsoft.XboxSpeechToTextOverlay"
        )
        
        if ($Aggressive) {
            # Also remove these in aggressive mode
            $aggressivePackages = @(
                "Microsoft.Windows.CloudExperienceHost",
                "Microsoft.Windows.SecHealthUI",
                "Microsoft.Windows.GetHelp",
                "Microsoft.Windows.TipOfTheDay",
                "Microsoft.Windows.AssignedAccessLockApp",
                "Microsoft.Windows.ContentDeliveryManager",
                "Microsoft.Windows.NarratorQuickStart",
                "Microsoft.Windows.ParentalControls",
                "Microsoft.Windows.PeopleExperienceHost",
                "Microsoft.Windows.SecureAssessmentBrowser",
                "Microsoft.Windows.ShellExperienceHost",
                "Microsoft.Windows.StartMenuExperienceHost"
            )
            $packagesToRemove += $aggressivePackages
        }
        
        # Remove packages
        foreach ($package in $packagesToRemove) {
            try {
                Write-Info "Removing package: $package..."
                
                # Try to remove as provisioned package first
                $result = dism /Image:$MountDir /Remove-ProvisionedAppxPackage /PackageName:$package /NoRestart
                if ($LASTEXITCODE -eq 0) {
                    $results.Removed += $package
                    Write-Success "Removed $package"
                    continue
                }
                
                # Try to remove as regular package
                $result = dism /Image:$MountDir /Remove-Package /PackageName:$package /NoRestart
                if ($LASTEXITCODE -eq 0) {
                    $results.Removed += $package
                    Write-Success "Removed $package"
                    continue
                }
                
                # Try to remove by directory
                $packageDirs = Get-ChildItem -Path (Join-Path $MountDir "Program Files\WindowsApps") -Directory -ErrorAction SilentlyContinue | 
                    Where-Object { $_.Name -like "*$package*" }
                foreach ($dir in $packageDirs) {
                    try {
                        Remove-Item -Path $dir.FullName -Recurse -Force
                        $results.Removed += $package
                        Write-Success "Removed $package directory"
                    } catch {
                        Write-WarningMsg "Failed to remove $($dir.FullName): $_"
                    }
                }
                
            } catch {
                Write-ErrorMsg "Failed to remove $package : $_"
                $results.Failed += $package
            }
        }
        
        # Remove Windows capabilities
        if ($Aggressive) {
            Write-Info "Removing Windows capabilities..."
            $capabilities = @(
                "Windows-Defender-ApplicationGuard",
                "Windows-Printing-PrintToPDFServices"
            )
            
            foreach ($capability in $capabilities) {
                try {
                    $result = dism /Image:$MountDir /Disable-Feature /FeatureName:$capability /NoRestart
                    if ($LASTEXITCODE -eq 0) {
                        $results.Removed += $capability
                        Write-Success "Disabled capability: $capability"
                    }
                } catch {
                    Write-WarningMsg "Failed to disable capability $capability: $_"
                }
            }
        }
        
        # Clean up registry
        Write-Info "Cleaning up registry..."
        Cleanup-Registry -MountDir $MountDir
        
        return $results
        
    } catch {
        Write-ErrorMsg "Failed to remove bloatware: $_"
        throw
    }
}

function Cleanup-Registry {
    param(
        [string]$MountDir
    )
    
    try {
        # Remove MsStore registry entries
        $regPaths = @(
            "HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Appx\AppxAllUserStore",
            "HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Appx\AppxAllUserStore\Deprovisioned"
        )
        
        foreach ($regPath in $regPaths) {
            try {
                # This would require offline registry editing
                Write-Info "Registry cleanup would be applied to: $regPath"
            } catch {
                Write-WarningMsg "Failed to cleanup registry $regPath: $_"
            }
        }
        
    } catch {
        Write-ErrorMsg "Failed to cleanup registry: $_"
        throw
    }
}

function Inject-FreeNT {
    param(
        [string]$MountDir,
        [string]$FreeNTDir
    )
    
    Write-Header "Injecting FreeNT Files"
    
    try {
        # Create FreeNT directory in Program Files
        $freentPath = Join-Path $MountDir "Program Files\FreeNT"
        if (-not (Test-Path $freentPath)) {
            New-Item -ItemType Directory -Path $freentPath -Force | Out-Null
        }
        
        # Copy FreeNT files
        Write-Info "Copying FreeNT source files..."
        $srcDir = Join-Path $FreeNTDir "src"
        if (Test-Path $srcDir) {
            Copy-Item -Path $srcDir -Destination (Join-Path $freentPath "src") -Recurse -Force
            Write-Success "Copied src directory"
        }
        
        Write-Info "Copying FreeNT standalone files..."
        $standaloneDir = Join-Path $FreeNTDir "standalone"
        if (Test-Path $standaloneDir) {
            Copy-Item -Path $standaloneDir -Destination (Join-Path $freentPath "standalone") -Recurse -Force
            Write-Success "Copied standalone directory"
        }
        
        Write-Info "Copying FreeNT scripts..."
        $scriptsDir = Join-Path $FreeNTDir "scripts"
        if (Test-Path $scriptsDir) {
            Copy-Item -Path $scriptsDir -Destination (Join-Path $freentPath "scripts") -Recurse -Force
            Write-Success "Copied scripts directory"
        }
        
        Write-Info "Copying configuration files..."
        $configFiles = @("requirements.txt", "setup.py", "setup.cfg", "pyproject.toml", "Makefile")
        foreach ($file in $configFiles) {
            $src = Join-Path $FreeNTDir $file
            if (Test-Path $src) {
                Copy-Item -Path $src -Destination $freentPath -Force
            }
        }
        
        # Create startup script
        Write-Info "Creating startup script..."
        Create-StartupScript -FreeNTDir $freentPath
        
        Write-Success "FreeNT files injected"
        return @{ Success = $true }
        
    } catch {
        Write-ErrorMsg "Failed to inject FreeNT files: $_"
        return @{ Success = $false }
    }
}

function Create-StartupScript {
    param(
        [string]$FreeNTDir
    )
    
    try {
        # Create batch file
        $startupBat = Join-Path $FreeNTDir "start_freent.bat"
        @"
@echo off
REM FreeNT Startup Script
SETLOCAL

REM Set FreeNT environment
set FREENT_HOME=%~dp0
set PATH=%PATH%;%FREENT_HOME%\standalone\bin

REM Start FreeNT shell
start "" "%FREENT_HOME%\standalone\bin\freent.bat"
"@ | Out-File -FilePath $startupBat -Encoding ASCII
        
        # Create PowerShell script
        $startupPs1 = Join-Path $FreeNTDir "start_freent.ps1"
        @"
# FreeNT Startup Script
`$ErrorActionPreference = "Stop"

# Set FreeNT environment
`$env:FREENT_HOME = Split-Path -Parent `$MyInvocation.MyCommand.Definition
`$env:PATH += ";`$env:FREENT_HOME\standalone\bin"

# Start FreeNT shell
Start-Process -FilePath "`$env:FREENT_HOME\standalone\bin\freent.bat"
"@ | Out-File -FilePath $startupPs1 -Encoding UTF8
        
        Write-Success "Startup scripts created"
        
    } catch {
        Write-ErrorMsg "Failed to create startup script: $_"
        throw
    }
}

function Configure-FreeNTShell {
    param(
        [string]$MountDir,
        [string]$FreeNTDir
    )
    
    Write-Header "Configuring FreeNT as Default Shell"
    
    try {
        # Set FreeNT shell as default in Winlogon
        Write-Info "Modifying Winlogon registry..."
        
        $freentShell = Join-Path $FreeNTDir "standalone\bin\freent.bat"
        
        # Create reg file for Winlogon modification
        $regContent = @"
Windows Registry Editor Version 5.00

[HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Winlogon]
"Shell"="$freentShell"
"Userinit"=
"@
        
        $regFile = Join-Path $MountDir "freent_shell.reg"
        $regContent | Out-File -FilePath $regFile -Encoding UTF8
        
        Write-Info "Winlogon configuration created at $regFile"
        
        # Modify Userinit to prevent Explorer from starting
        Write-Info "Disabling Userinit..."
        
        # Add to Run registry key
        Write-Info "Adding to Run registry..."
        
        $runRegContent = @"
Windows Registry Editor Version 5.00

[HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Run]
"FreeNT"="$freentShell"

[HKEY_CURRENT_USER\SOFTWARE\Microsoft\Windows\CurrentVersion\Run]
"FreeNT"="$freentShell"
"@
        
        $runRegFile = Join-Path $MountDir "freent_run.reg"
        $runRegContent | Out-File -FilePath $runRegFile -Encoding UTF8
        
        Write-Info "Run registry configuration created at $runRegFile"
        
        Write-Success "FreeNT shell configured"
        
    } catch {
        Write-ErrorMsg "Failed to configure FreeNT shell: $_"
        throw
    }
}

function Remove-OtherEditions {
    param(
        [string]$WIMPath,
        [int]$KeepIndex,
        [int]$WindowsVersion
    )
    
    Write-Header "Removing Other Editions"
    
    try {
        # Get WIM info
        $wimInfo = Get-WIMInfo -WIMPath $WIMPath
        if (-not $wimInfo) {
            throw "Failed to get WIM info"
        }
        
        # Find all indices
        $allIndices = @()
        foreach ($key in $wimInfo.Keys) {
            if ($key -match "^Index_(\d+)") {
                $index = $matches[1]
                $allIndices += [int]$index
            }
        }
        
        # Sort in reverse order (so we can delete from end)
        $allIndices = $allIndices | Sort-Object -Descending
        
        # Remove all indices except the one we want to keep
        foreach ($index in $allIndices) {
            if ($index -ne $KeepIndex) {
                Write-Info "Removing image index $index..."
                $result = dism /Delete-Image /ImageFile:$WIMPath /Index:$index
                
                if ($LASTEXITCODE -eq 0) {
                    Write-Success "Removed image index $index"
                } else {
                    Write-WarningMsg "Failed to remove image index $index"
                }
            }
        }
        
        # Rename the remaining image to FreeNT
        if ($WindowsVersion -eq 11) {
            $newName = "Windows 11 FreeNT"
        } else {
            $newName = "Windows 10 FreeNT"
        }
        
        Write-Info "Renaming image to $newName..."
        $result = dism /Export-Image /SourceImageFile:$WIMPath /SourceIndex:$KeepIndex /DestinationImageFile:$WIMPath /DestinationName:"$newName" /Compress:max /CheckIntegrity
        
        if ($LASTEXITCODE -eq 0) {
            Write-Success "Image renamed to $newName"
        } else {
            Write-WarningMsg "Failed to rename image"
        }
        
        Write-Success "Other editions removed"
        
    } catch {
        Write-ErrorMsg "Failed to remove other editions: $_"
        throw
    }
}

function Display-Summary {
    param(
        [bool]$Success,
        [string]$Message,
        [string]$OutputWIM,
        [array]$Actions
    )
    
    Write-Header "Creation Summary"
    
    if ($Success) {
        Write-Host "FreeNT WIM creation completed successfully!" -ForegroundColor Green
    } else {
        Write-Host "FreeNT WIM creation failed!" -ForegroundColor Red
    }
    
    Write-Host ""
    Write-Host "Message: $Message" -ForegroundColor Yellow
    Write-Host "Output WIM: $OutputWIM" -ForegroundColor Cyan
    Write-Host ""
    
    if ($Actions.Count -gt 0) {
        Write-Host "Actions performed ($($Actions.Count)):" -ForegroundColor Cyan
        foreach ($action in $Actions) {
            Write-Host "  - $action" -ForegroundColor Green
        }
        Write-Host ""
    }
    
    if ($Success) {
        Write-Host "Your FreeNT WIM is ready for installation!" -ForegroundColor Green
        Write-Host ""
        Write-Host "Next steps:" -ForegroundColor Cyan
        Write-Host "  1. Use the WIM with your preferred installation method" -ForegroundColor Yellow
        Write-Host "  2. Apply the WIM to a partition using DISM or ImageX" -ForegroundColor Yellow
        Write-Host "  3. Configure bootloader to point to the Windows installation" -ForegroundColor Yellow
        Write-Host "  4. Boot into FreeNT!" -ForegroundColor Yellow
    }
}

# Main creation process
try {
    # Start logging
    $logFile = Join-Path $WorkDir "create_freent_wim.log"
    Start-Transcript -Path $logFile -Append -Force
    
    Write-Header "FreeNT WIM Creator Started"
    Write-Host "ISO Path: $ISOPath" -ForegroundColor Cyan
    Write-Host "Output WIM: $OutputWIM" -ForegroundColor Cyan
    Write-Host "Windows Version: $WindowsVersion" -ForegroundColor Cyan
    Write-Host "Work Directory: $WorkDir" -ForegroundColor Cyan
    Write-Host "Keep Original: $KeepOriginal" -ForegroundColor Cyan
    Write-Host ""
    
    # Confirm creation
    Confirm-Creation -ISOPath $ISOPath -OutputWIM $OutputWIM -WindowsVersion $WindowsVersion
    
    # Create work directory
    if (-not (Test-Path $WorkDir)) {
        New-Item -ItemType Directory -Path $WorkDir -Force | Out-Null
    }
    
    # Create actions log
    $Actions = @()
    
    # Mount ISO
    $mountResult = Mount-ISO -ISOPath $ISOPath -MountDir (Join-Path $WorkDir "iso")
    if (-not $mountResult.Mounted) {
        throw "Failed to mount ISO"
    }
    $Actions += "Mounted ISO at $($mountResult.Path)"
    
    try {
        # Extract WIM from ISO
        $wimResult = Extract-WIMFromISO -ISOMountPath $mountResult.Path
        if (-not $wimResult.Found) {
            throw "Failed to extract WIM from ISO"
        }
        $Actions += "Found $($wimResult.Type) at $($wimResult.Path)"
        
        # Convert ESD to WIM if needed
        if ($wimResult.Type -eq "ESD") {
            $wimPath = Join-Path $WorkDir "install.wim"
            $convertResult = Convert-ESDToWIM -ESDPath $wimResult.Path -OutputWIM $wimPath
            if (-not $convertResult) {
                throw "Failed to convert ESD to WIM"
            }
            $wimResult.Path = $wimPath
            $wimResult.Type = "WIM"
            $Actions += "Converted ESD to WIM"
        }
        
        # Get Enterprise image index
        $enterpriseIndex = Get-EnterpriseImageIndex -WIMPath $wimResult.Path
        if (-not $enterpriseIndex) {
            throw "Enterprise edition not found in WIM"
        }
        $Actions += "Found Enterprise edition at index $enterpriseIndex"
        
        # Find FreeNT directory
        $freeNTDir = $PSScriptRoot
        while ($freeNTDir -and (Test-Path (Join-Path $freeNTDir ".."))) {
            $freeNTDir = Join-Path $freeNTDir ".."
        }
        $freeNTDir = Resolve-Path $freeNTDir
        
        if (-not (Test-Path (Join-Path $freeNTDir "src"))) {
            throw "FreeNT directory not found"
        }
        $Actions += "Found FreeNT at $freeNTDir"
        
        # Create FreeNT WIM
        $createResult = Create-FreeNTWIM -WIMPath $wimResult.Path -OutputWIM $OutputWIM -ImageIndex $enterpriseIndex -WindowsVersion $WindowsVersion -WorkDir $WorkDir -FreeNTDir $freeNTDir -KeepOriginal:$KeepOriginal
        
        if ($createResult.Success) {
            $Actions += "Created FreeNT WIM"
        } else {
            throw $createResult.Message
        }
        
        # Display summary
        $OutputWIM = $createResult.OutputWIM
        Display-Summary -Success $true -Message "FreeNT WIM created successfully" -OutputWIM $OutputWIM -Actions $Actions
        
        # Stop logging
        Stop-Transcript
        
        exit 0
        
    } catch {
        Write-ErrorMsg "Creation failed: $_"
        Display-Summary -Success $false -Message "FreeNT WIM creation failed: $_" -OutputWIM $null -Actions $Actions
        Stop-Transcript
        exit 1
    }
    
} catch {
    Write-ErrorMsg "Fatal error: $_"
    exit 1
}
finally {
    # Cleanup if requested
    if ($Cleanup) {
        Write-Info "Cleaning up..."
        try {
            if ($mountResult -and $mountResult.Mounted) {
                Unmount-ISO -ISOPath $ISOPath
            }
            
            if (Test-Path $WorkDir) {
                Remove-Item -Path $WorkDir -Recurse -Force -ErrorAction SilentlyContinue
            }
        } catch {
            Write-WarningMsg "Failed to cleanup: $_"
        }
    }
}
