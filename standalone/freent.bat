@echo off
REM FreeNT Standalone Launcher
REM Copyright (c) 2026, Panoc95
REM BSD 3-Clause License

REM This batch file launches FreeNT in standalone mode
REM It works with or without Python installed system-wide

SETLOCAL ENABLEDELAYEDEXPANSION

REM Set standalone directory
SET "STANDALONE_DIR=%~dp0"

REM Check if Python is available
python --version >nul 2>&1
IF %ERRORLEVEL% EQU 0 (
    REM Python is available system-wide
    python "%STANDALONE_DIR%..\src\login_manager\login_app.py" %*
    GOTO :EOF
)

REM Check for portable Python in standalone directory
IF EXIST "%STANDALONE_DIR%python\python.exe" (
    "%STANDALONE_DIR%python\python.exe" "%STANDALONE_DIR%..\src\login_manager\login_app.py" %*
    GOTO :EOF
)

REM Check for Python in common portable locations
IF EXIST "%STANDALONE_DIR%..\python\python.exe" (
    "%STANDALONE_DIR%..\python\python.exe" "%STANDALONE_DIR%..\src\login_manager\login_app.py" %*
    GOTO :EOF
)

REM Check in PATH
WHERE python >nul 2>&1
IF %ERRORLEVEL% EQU 0 (
    python "%STANDALONE_DIR%..\src\login_manager\login_app.py" %*
    GOTO :EOF
)

REM Python not found
ECHO Error: Python is required to run FreeNT
ECHO Please install Python or place a portable Python in the standalone directory
ECHO Download Python from: https://www.python.org/downloads/
PAUSE
GOTO :EOF
