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

The default edition is `Full`. To build and launch the full game with the imported retail assets:

```powershell
.\BUILD\BUILD.PS1 -Edition Full -Configuration Release -Test
```

Full executables are published under `GAME`; Demo executables under `GAME/DEMO`. Use `-Edition Demo` to build and test the demo. Intermediate files are isolated under `BUILD/OUT/<Edition>`. Edition and optimization configuration are independent: `Full` selects the full-game code paths and assets, while `Release` selects compiler flags. The legacy demo macro changes class layouts, so each edition is compiled separately rather than switched inside one running executable. See `GAME/README.MD` for required retail resources and current limitations.

Every invocation builds the Tigre library first and then links BAM. Objects, maps, libraries, and the intermediate executable are written below `BUILD/OUT` and ignored by Git. The finished executable is copied into the runnable `GAME` directory as `BAM.EXE` for Release or `BAMDBG.EXE` for Debug; both generated files are also ignored by Git.

The compatibility target remains Watcom-family C++ for 32-bit DOS/4GW. Important compatibility options include signed `char`, the flat 386 memory model, and byte-packed structures (`/zp1`) required by `MAP.IDX`.

## BUILD AND TEST

To build and immediately run the game:

```powershell
.\BUILD\BUILD.PS1 -Configuration Release -Test
```

The script uses `TOOLS/DOSBOX/DOSBox.exe`, mounts `GAME` as drive `C:`, and launches the executable for the selected configuration. It falls back to a DOSBox executable on `PATH` only if the bundled copy is absent.

The verified Release build reaches the main scenario-selection menu without user intervention. Cinematics currently advance immediately because the legacy Smacker decoder is disabled. Sound and multiplayer are also disabled in code while their recoverable audio assets, SDK libraries, headers, DOS drivers, and configuration files remain in the tree.

## BUILD RESOURCES

To regenerate the packed resource set without replacing the runnable copies:

```powershell
.\BUILD\BUILD-RESOURCES.PS1
```

The resource builder currently rebuilds the demo archive from `GAME/DEMO/DATA`; loose retail resource inputs are not available. The results and packer log are written under `BUILD/OUT/RESOURCES`. Add `-Publish` to copy the verified `MAIN.STF`, `MAIN.MAP`, and `MAP.IDX` into `GAME/DEMO`. The script checks that every file named by `TOOLS/RESOURCE/STUFF.DAT` exists before invoking the original `MAKESTUF.EXE` through the bundled DOSBox. It does not overwrite the retail archives in `GAME`.

## RUNTIME FILES

The runnable distribution is kept under `GAME`. The build verifies these required files before publishing the selected executable into the same directory:

- `DOS4GW.EXE`
- `MAIN.STF`
- `MAIN.MAP`
- `MAP.IDX`
- `RES_CFG.HPP`
- `SOUND.CFG`

`GAME/MAIN.STF` contains the full-game packed resources. `GAME/DEMO/MAIN.STF` and the loose development assets under `GAME/DEMO/DATA` belong to the demo. Numeric names such as `9050.FON` are resource identifiers and intentionally retain their original spelling.

If startup reports that `9050.FON` is missing, BAM did not successfully open the packed resource set. Run the published executable through `-Test`; do not copy `BAM.EXE` or `BAMDBG.EXE` elsewhere without its runtime files.

## SOURCE LAYOUT

- `CODE/TIGRE`: active Tigre source and headers.
- `CODE/SRC`: active game source and headers.
- `CODE/TIGRE/TIGRE.MAK`, `CODE/SRC/BAM.MAK`, and `CODE/SRC/BAMF.MAK`: preserved historical makefiles.
- `CODE/SRC/MAPEDIT*`: original map editor source.
- `MAKERES`, `RESINFO`, `MAKEMIF`, `TIL2TLB`, and `TXT2SQB` sources: original content/resource pipeline.
- `TOOLS/RESOURCE/MAKESTUF.EXE` and `STUFF.DAT`: original final archive packer and complete 828-file manifest.

The historical makefiles are useful specifications but are not yet the supported build entry point. Modernizing the content pipeline—without networking and communications dependencies—is future work.

## CURRENT RESTORATION BOUNDARY

- Single-player game logic, Mode X graphics, keyboard, mouse, resource loading, and DOS/4GW execution are active.
- Network transport and multiplayer are intentionally out of scope.
- The original `SOUNDMGR_SOS.*` implementation and SOS libraries are active. The demo configuration selects Sound Blaster 16 digital audio and OPL3 music. Driver initialization and WAV/HMP playback are logged to `AUDIO.LOG` while restoration is being tested. Retail audio configuration still needs verification.
- The original `FLICSMK_SMACKER.*` wrapper and `SMACK.LIB` are active again, using the SOS digital driver. Complete movie playback and sound still need verification.

### DOSBox audio troubleshooting

`-Test` defaults to SDL's Windows `waveout` audio backend. Use
`-DosBoxAudioDriver dsound` or `-DosBoxAudioDriver auto` to test alternatives.
The override is local to the launched process; it does not change Windows environment settings.

`BUILD/DOSBOX.CONF` uses fixed 30,000 CPU cycles. The initial 100 ms prebuffer
with 2,048-sample blocks improved the initial full-version startup test,
but crackling was subsequently reported when another application also played
audio. Backend/buffering investigation is ongoing; this is not yet a confirmed
fix. A 200 ms prebuffer with 4,096-sample blocks is now being tested. The larger
buffer adds audio latency.

Full-version movies and scripts search `GAME` before `GAME/STFS`. Both contain
`30.SMK`, but the root copy is the longer opening cinematic; the `STFS` copy is
much shorter. Campaign movies still fall back to `STFS`.
By choice, `GAME/DEMO/30.SMK` uses the short variant (206,160 bytes), while
`GAME/30.SMK` retains the long intro (4,318,552 bytes). This is an edition
presentation choice, not a claim about the original release arrangement.

If DOSBox prints `Can't open audio: No available audio device`, it has disabled
host audio output even if BAM successfully initializes its emulated sound card.
In particular, an inherited `SDL_AUDIODRIVER=directsound` is invalid for bundled
SDL 1.2: its DirectSound backend is named `dsound`. For a manual launch, run
`$env:SDL_AUDIODRIVER = 'waveout'` in that PowerShell session first.

### Save/load restoration

The original save manager is re-enabled: its historical release-build bypass
previously reported success without writing anything. Single-player slots use
`1.SAV` through `10.SAV` in the selected edition's runtime directory. These
files and generated campaign progress are ignored by Git.

Saves contain raw engine object layouts, so use the same edition, executable
build, and configuration to save and load. Header version/build mismatches are
rejected before restoring allocations. Compatibility with original commercial
save files is not guaranteed. In-game save/load round-trip verification is
still pending.

See `DOCS/LEGACY_NOTES.MD` for the investigation record.
