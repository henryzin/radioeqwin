param(
    [string]$Root = (Split-Path -Parent $PSScriptRoot),
    [string]$Output = "radioeq.exe"
)

$ErrorActionPreference = "Stop"

if (-not (Get-Command g++ -ErrorAction SilentlyContinue)) {
    throw "MinGW/GCC not found. Install MSYS2 UCRT64 and ensure g++ is in PATH."
}

Set-Location $Root

& g++ -std=c++17 -O2 -Wall -Wextra -mwindows -I"$Root/source" "$Root/source/main.cpp" -o "$Root/$Output" -lwinmm -lole32 -lcomdlg32

if ($LASTEXITCODE -ne 0) {
    throw "Windows build failed."
}

Write-Host "Build created: $Root/$Output"
