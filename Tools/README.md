# Local development tools

`Build/Build.ps1` automatically detects an Open Watcom 1.9 installation at `Tools/open-watcom-1.9`.

The official Windows C/C++ installer was downloaded from:

```text
https://github.com/open-watcom/open-watcom-1.9/releases/download/ow1.9/open-watcom-c-win32-1.9.exe
```

Verified installer properties:

```text
Size: 84,012,543 bytes
MD5:  6316F454F732B0705EBFE2A278DC1E59
```

The installer is a self-extracting ZIP archive. Its contents were extracted directly into `Tools/open-watcom-1.9`; no machine-wide installation or registry changes were made. Both the extracted toolchain and downloaded installer are intentionally ignored by Git. The vendor package name is intentionally retained rather than restyled as a repository-owned PascalCase name.
