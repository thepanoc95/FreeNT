@echo off

REM === FreeNT TUI Installer Launcher ===
REM Runs the PDCurses-based TUI installer in a WinPE environment.
REM The WinPE runtime lives on X:, the target disk gets assigned C:.

echo Starting FreeNT Installer...
echo (TUI mode - close the installer to return to WinPE)
echo.

freent_installer.exe
if errorlevel 1 (
    echo.
    echo Installer reported an error.
    pause
    exit /b 1
)

pause
exit /b 0
