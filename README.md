# Radio EQ Windows Native

This is the easy version for normal Windows users: download a portable folder, run one program, and it should work without technical setup.

## What it does

- plays local audio files
- plays YouTube links
- applies a simple Bass Boost filter
- works as a compact Win32 app

## The user flow

1. Download the ZIP
2. Extract it anywhere
3. Double-click the main app
4. Paste a YouTube link or choose a local file
5. Click Play

## Important for a beginner

The app is designed to be portable. The ideal setup is:

- a single folder
- one executable file
- no compilation needed
- no terminal commands needed
- no Git or build tools needed

## Required tools for the runtime

For YouTube and conversion support, the app expects these files in the same folder:

- `yt-dlp.exe`
- `ffmpeg.exe`

If they are missing, use the helper script:

- `fix_tools.cmd`

This script downloads the official binaries and places them next to the app.

## Start normally

Double-click:

- `radioeq.exe`

If you do not have the external tools yet, double-click:

- `fix_tools.cmd`

Then run `radioeq.exe` again.

## Why this is simpler

This avoids the scary command-line build process. The app is meant to be a finished Windows program, not a developer project.

## Notes

- This repo is a clean Windows-native rewrite.
- The goal is to ship a user-friendly package, not a technical developer environment.
- The build helpers remain in the repo for maintainers, but the end user should only use the executable.
