@echo off
setlocal
set "ROOT=%~dp0"

if exist "%ROOT%radioeq.exe" (
    start "" "%ROOT%radioeq.exe"
    exit /b 0
)

echo.
echo Radio EQ was not found in this folder.
echo Please place radioeq.exe next to this file and run it again.
echo.
pause
exit /b 1
