@echo off
REM FreeNT CLI Launcher
REM Copyright (c) 2026, Panoc95
REM BSD 3-Clause License

SETLOCAL ENABLEDELAYEDEXPANSION

REM Set standalone directory
SET "STANDALONE_DIR=%~dp0"

REM Check if Python is available
python --version >nul 2>&1
IF %ERRORLEVEL% EQU 0 (
    python "%STANDALONE_DIR%..\src\cli.py" %*
    GOTO :EOF
)

REM Check for portable Python
IF EXIST "%STANDALONE_DIR%python\python.exe" (
    "%STANDALONE_DIR%python\python.exe" "%STANDALONE_DIR%..\src\cli.py" %*
    GOTO :EOF
)

REM Check in PATH
WHERE python >nul 2>&1
IF %ERRORLEVEL% EQU 0 (
    python "%STANDALONE_DIR%..\src\cli.py" %*
    GOTO :EOF
)

ECHO Error: Python is required to run FreeNT CLI
PAUSE
GOTO :EOF
