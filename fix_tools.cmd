@echo off
setlocal
set "ROOT=%~dp0"
set "YT=%ROOT%yt-dlp.exe"
set "FF=%ROOT%ffmpeg.exe"

if not exist "%YT%" (
  echo Downloading yt-dlp... 
  powershell -NoProfile -ExecutionPolicy Bypass -Command "Invoke-WebRequest -Uri 'https://github.com/yt-dlp/yt-dlp/releases/latest/download/yt-dlp.exe' -OutFile '%YT%'"
)

if not exist "%FF%" (
  echo Downloading ffmpeg... 
  powershell -NoProfile -ExecutionPolicy Bypass -Command "$ProgressPreference = 'SilentlyContinue'; Invoke-WebRequest -Uri 'https://www.ffmpeg.org/download.html' -UseBasicParsing -OutFile '%ROOT%ffmpeg_download_page.html'"
  echo.
  echo Please download ffmpeg from the official page and place ffmpeg.exe in this folder.
  echo Official page: https://www.ffmpeg.org/download.html
  echo.
  pause
  exit /b 0
)

echo Tools are ready.
pause
