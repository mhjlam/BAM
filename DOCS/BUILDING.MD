# BUILDING BLOOD & MAGIC FOR DOS

This branch builds the 32-bit DOS game with the bundled Open Watcom 1.9 toolchain and runs it with the bundled DOSBox 0.74-3 runtime. Neither tool needs to be installed globally.

## BUILD

From PowerShell at the repository root:

```powershell
.\BUILD\BUILD.PS1
```

The default configuration is `Debug`. A clean Release build is:

```powershell
.\BUILD\BUILD.PS1 -Configuration Release -Rebuild
```

Every invocation builds the Tigre library first and then links BAM. Generated objects, maps, libraries, executables, and staged runtime files are written below `BUILD/OUT` and ignored by Git.

The compatibility target remains Watcom-family C++ for 32-bit DOS/4GW. Important compatibility options include signed `char`, the flat 386 memory model, and byte-packed structures (`/zp1`) required by `MAP.IDX`.

## BUILD AND TEST

To build and immediately run the game:

```powershell
.\BUILD\BUILD.PS1 -Configuration Release -Test
```

The script uses `TOOLS/DOSBOX/DOSBox.exe`, mounts the staged output directory as drive `C:`, and launches `BAM.EXE`. It falls back to a DOSBox executable on `PATH` only if the bundled copy is absent.

The verified Release build reaches the main scenario-selection menu without user intervention. Cinematics currently advance immediately because the legacy Smacker decoder is disabled. Sound and multiplayer are also disabled in code while their recoverable audio assets, SDK libraries, headers, DOS drivers, and configuration files remain in the tree.

## RUNTIME FILES

The build stages these files beside `BAM.EXE`:

- `DOS4GW.EXE`
- `MAIN.STF`
- `MAIN.MAP`
- `MAP.IDX`
- `RES_CFG.HPP`
- `SOUND.CFG`

`MAIN.STF` contains the packed resources used during normal play. The files under `DATA` are the corresponding loose development assets. Numeric names such as `9050.FON` are resource identifiers and intentionally retain their original spelling.

If startup reports that `9050.FON` is missing, BAM did not successfully open the packed resource set. Run the staged executable through `-Test`; do not copy `BAM.EXE` elsewhere without its runtime files.

## SOURCE LAYOUT

- `CODE/TIGRE`: active Tigre source and headers.
- `CODE/SRC`: active game source and headers.
- `CODE/TIGRE/TIGRE.MAK`, `CODE/SRC/BAM.MAK`, and `CODE/SRC/BAMF.MAK`: preserved historical makefiles.
- `CODE/SRC/MAPEDIT*`: original map editor source.
- `MAKERES`, `RESINFO`, `MAKEMIF`, `TIL2TLB`, and `TXT2SQB` sources: original content/resource pipeline.

The historical makefiles are useful specifications but are not yet the supported build entry point. Modernizing the content pipeline—without networking and communications dependencies—is future work.

## CURRENT RESTORATION BOUNDARY

- Single-player game logic, Mode X graphics, keyboard, mouse, resource loading, and DOS/4GW execution are active.
- Network transport and multiplayer are intentionally out of scope.
- Audio restoration is the next planned subsystem. SOS libraries, HMI drivers, banks, MIDI/HMP files, WAV files, setup data, configuration, `8TRACK.*`, and the original `SOUNDMGR_SOS.*` implementation have therefore been retained. The active build continues to compile the disabled `SOUNDMGR.*` facade.
- Smacker media, its binary SDK files, and the original `FLICSMK_SMACKER.*` wrapper are retained. The active build compiles the disabled `FLICSMK.*` facade because the old decoder crashes on part of the intro sequence.

See `DOCS/LEGACY_NOTES.MD` for the investigation record.
