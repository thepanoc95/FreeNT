@echo off
REM FreeNT Startup Script
REM Copyright (c) 2026, Panoc95
REM BSD 3-Clause License

REM This script sets up environment variables and launches FreeNT

SETLOCAL ENABLEDELAYEDEXPANSION

REM Set standalone directory
SET "STANDALONE_DIR=%~dp0"

REM Set FreeNT environment variables
SETX FREENT_HOME "%STANDALONE_DIR%.." /M >nul 2>&1
SET FREENT_HOME=%STANDALONE_DIR%..

REM Add standalone bin to PATH if not already there
ECHO %PATH% | FIND /I "%STANDALONE_DIR%bin" >nul 2>&1
IF %ERRORLEVEL% NEQ 0 (
    SETX PATH "%PATH%;%STANDALONE_DIR%bin" /M >nul 2>&1
)

REM Check for portable Python and add to PATH
IF EXIST "%STANDALONE_DIR%python" (
    SETX PATH "%PATH%;%STANDALONE_DIR%python" /M >nul 2>&1
)

REM Launch the login manager
CALL "%STANDALONE_DIR%freent.bat"
