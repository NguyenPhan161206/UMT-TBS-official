@echo off
setlocal enabledelayedexpansion

set PROJECT_DIR=%~dp0
set ROOT_DIR=%PROJECT_DIR%..\..

:: 1. Find PlatformIO
where pio >nul 2>nul
if %ERRORLEVEL% equ 0 (
    set PIO_CMD=pio
) else if exist "%USERPROFILE%\.platformio\penv\Scripts\pio.exe" (
    set PIO_CMD="%USERPROFILE%\.platformio\penv\Scripts\pio.exe"
) else (
    echo Error: 'pio' command not found!
    echo Please install PlatformIO or run this script inside the PlatformIO terminal.
    exit /b 1
)

echo Using PlatformIO at: %PIO_CMD%

cd /d "%PROJECT_DIR%"

:: Flash using auto-detect port (PlatformIO is usually smart enough on Windows)
echo Flashing with auto-detect port...
%PIO_CMD% run -e yolo_uno -t upload
