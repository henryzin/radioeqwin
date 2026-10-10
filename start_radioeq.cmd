@echo off
setlocal
set "ROOT=%~dp0"
if exist "%ROOT%radioeq.exe" (
  start "" "%ROOT%radioeq.exe"
  exit /b 0
)

echo The app file radioeq.exe was not found in this folder.
echo Please build it first or place the final executable here.
pause
