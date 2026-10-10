@echo off
setlocal
set ROOT=%~dp0
where g++ >nul 2>nul
if errorlevel 1 (
  echo MinGW/GCC not found. Install MSYS2 UCRT64 and ensure g++ is in PATH.
  exit /b 1
)

g++ -std=c++17 -O2 -Wall -Wextra -mwindows -I"%ROOT%source" "%ROOT%source\main.cpp" -o "%ROOT%radioeq.exe" -lwinmm -lole32 -lcomdlg32
if errorlevel 1 (
  echo Windows build failed.
  exit /b 1
)

echo Build created: %ROOT%radioeq.exe
