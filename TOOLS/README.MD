# LOCAL DEVELOPMENT TOOLS

This branch is self-contained on Windows:

- `WATCOM` contains Open Watcom 1.9. `BUILD/BUILD.PS1` initializes it without a machine-wide installation.
- `DOSBOX` contains DOSBox 0.74-3 and the SDL runtime used by the test target.
- `RESOURCE` contains the original `MAKESTUF.EXE` archive builder and its manifest.

The Open Watcom package came from the official Open Watcom 1.9 Windows C/C++ distribution. Its license and installation notes are retained inside `WATCOM`.

The DOSBox binary, documentation, GPL text, and matching source archive are retained together under `DOSBOX`; see `DOSBOX/README.MD`.
