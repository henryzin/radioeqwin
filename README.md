# Radio EQ Windows Native

This project is a full Windows-native rewrite of the Radio EQ idea: the app downloads or opens a local audio file, applies a Bass Boost filter, and plays it back using Windows APIs.

## Core idea

- The app works with direct local files or YouTube links.
- YouTube downloads are handled by `yt-dlp`.
- Audio conversion is handled by `ffmpeg`.
- The output signal is processed with a simple bass-boost DSP filter.
- Playback is done with native Windows audio APIs.

## Tools used

- C++17
- Win32 API
- `yt-dlp` for YouTube extraction
- `ffmpeg` for conversion to WAV
- `g++` with MinGW/MSYS2 on Windows

## Build on Windows

### 1) Install MSYS2 UCRT64

```bash
pacman -S --needed mingw-w64-ucrt-x86_64-toolchain zip
```

### 2) Install external tools

Install `yt-dlp` and `ffmpeg` and ensure they are in PATH.

### 3) Build

PowerShell:

```powershell
powershell -ExecutionPolicy Bypass -File .\build_windows.ps1
```

Or batch:

```cmd
build_windows.bat
```

### 4) Run

```cmd
radioeq.exe
```

## Behavior

- Enter a local file path or a YouTube URL.
- Click Play.
- The app downloads or prepares the audio in a temp folder.
- A Bass Boost effect is applied to the PCM stream.
- The boosted WAV is then played using Windows sound APIs.

## Notes

This is a clean rewrite from scratch for Windows and is intentionally independent from the Linux/FLTK version.
