@echo off
setlocal
set "ROOT=%~dp0"

where winget >nul 2>nul
if errorlevel 1 (
    echo Winget not found.
    echo Please install the Microsoft Store package for App Installer or use another method to install FFmpeg.
    echo.
    pause
    exit /b 1
)

if not exist "%ROOT%yt-dlp.exe" (
    echo Downloading yt-dlp.exe...
    powershell -NoProfile -ExecutionPolicy Bypass -Command "Invoke-WebRequest -Uri 'https://github.com/yt-dlp/yt-dlp/releases/latest/download/yt-dlp.exe' -OutFile '%ROOT%yt-dlp.exe'"
)

where ffmpeg >nul 2>nul
if errorlevel 1 (
    echo Installing FFmpeg with winget...
    winget install --id Gyan.Dev.FFmpeg -e
)

if not exist "%ROOT%yt-dlp.exe" (
    echo yt-dlp still missing.
    echo Download it manually and place it in this folder.
)

where ffmpeg >nul 2>nul
if not errorlevel 1 (
    echo.
    echo Everything is ready.
    echo You can now double-click start.bat
    echo.
    pause
    exit /b 0
)

echo.
echo FFmpeg is not available yet. Please install it and run start.bat again.
echo.
pause
