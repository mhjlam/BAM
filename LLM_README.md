# Blood & Magic — Linux Port

Porting the 1996 DOS game *Blood & Magic* (Tachyon Studios/Interplay) to Linux x86-64.
The game uses the Tachyon TIGRE engine, originally written in Watcom C++ 386 for 32-bit DOS.

## Status

The game is fully playable on Linux. The binary works as a drop-in replacement in a real
game distribution directory, reading all resources from `MAIN.STF`. Tested on Fedora 44
(x86-64, aarch64).

What has been done:

- CMake build system replacing the original Watcom Make files
- Compatibility shim layer in `CODE/compat/` for DOS/Watcom-specific headers and language
  extensions; DOS keyword macros (`near`, `far`, `huge`, `cdecl`, `pascal`, `interrupt`)
  removed from all source files
- **Graphics**: SDL3 GPU pipeline in `sdl_backend.cpp`. Three R8_UNORM indexed
  textures (world, battlefield, UI) rendered by a GLSL palette-lookup shader — no CPU
  indexed→RGBA conversion. `0xFE` pixels are transparent, letting lower layers show through.
  Background map tiles stored in a GPU-resident world texture, scrolled without rebaking.
  Full-frame compositor in `GraphicsMgr::Animate()` replaces the original dirty-rect system.
  Resizable window maintains 4:3 aspect ratio (pillarbox/letterbox); corrects the original
  non-square CRT pixels (Mode X 320×400 on a 4:3 screen).
- **Input**: `ASDLPumpEvents()` translates SDL3 keyboard (full DOS scancode map)
  and mouse events into the TIGRE event queue. Mouse coordinates mapped through the 4:3
  output rect to remain correct at any window size.
- **Audio**: WAV sound effects and HMP/NDMF MIDI music via SDL3_mixer with
  FluidSynth. `hmp_to_smf()` converts HMI's HMIMIDIP format to SMF in-memory. Requires a
  SoundFont (e.g. `fluid-soundfont-gm`). Volume settings persisted in `config.toml`.
- **FMV**: Smacker (`.SMK`) player in `flicsmk.cpp`/`cine.cpp`.
  Video, audio, and subtitles all work. Implemented via `smacker.cpp` in `CODE/compat/`
  wrapping libsmacker.
- **Resource loading**: case-insensitive `ci_fopen()` in `file.cpp`; DOS
  absolute paths filtered in `resmgr.cpp`; `#pragma pack(1)` on disk-mapped structs
  (`ResInfo`, `MapRec`, `MapIdxRec`, `ChunkInfo`) to match Watcom's 1-byte enum layout.
- **Save/Load**: explicit serialiser using POSIX file I/O in `savemgr.cpp`.
- **Networking**: ENet/UDP lockstep multiplayer via `TEnetComm`.
  Launch with `-NET host` or `-NET <ip>`; port 7733. Settings synced via `NetcharPacket`.
  Command-line and UI.
- **Configuration**: TOML config file in the XDG config directory (`GameConfig` in
  `game_config.hpp/cpp`). Stores audio volumes, game speed, and other settings.
- **Game speed**: Normal / Fast / Fastest via `TClock::SetSpeed`; UI in the options menu.

## Original build environment

- Compiler: Watcom C++ 386 (32-bit DOS, DPMI protected mode)
- Engine: Tachyon TIGRE
- Build system: Watcom Make (`.MAK` files in `CODE/TIGRE/` and `CODE/SRC/`)

## Repository layout

```
CODE/
  TIGRE/      TIGRE engine source (C++ and headers)
  SRC/        Game source
  compat/     Porting compatibility layer (headers, stubs, SDL3 backend)
docs/         Porting notes and architecture documentation
build/        CMake build directory (out-of-tree)
```

### Compatibility layer (`CODE/compat/`)

| File | Purpose |
|------|---------|
| `portability.h` | Force-included by CMake; provides Linux stubs for DOS/Watcom APIs (`_disable`/`_enable`, `FP_SEG`/`FP_OFF`/`MK_FP`, `REGS`/`SREGS`/`int386`, `lock_region`, `itoa`, `strupr`, `strlwr`, `delay`) |
| `i86.h` | Empty stub — all `REGS`/`int386` definitions live in `portability.h` |
| `dos.h` | Empty stub |
| `conio.h` | Stubs for `kbhit`, `getch`, `putch`, `ungetch` |
| `sdl_backend.cpp` | SDL3 GPU backend: window, SPIR-V shader pipeline, R8 indexed textures, world texture API (`OS_WorldTexture*`), event pump (`ASDLPumpEvents`), palette animation |
| `shaders/` | GLSL sources (`blit.vert.glsl`, `palette_blit.frag.glsl`), compiled to SPIR-V at build time |
| `smacker.cpp` | Smacker video API wrapping libsmacker for FMV playback |

## Build

Dependencies: CMake 3.16+, SDL3, SDL3_mixer (with FluidSynth support), ENet, libsmacker, GCC or
Clang with C++17 support.

```sh
mkdir build && cd build
cmake ..
make -j8
```

Produces `build/bam`. Run from the game's data directory (which contains `MAIN.STF`),
or use `-datadir <path>` / set `BAM_DATA=<path>`.

## Notable porting issues

**Case sensitivity.** All source files were lowercased. At runtime, `ci_fopen()` does a
case-insensitive directory scan so uppercase data files (e.g. `MAIN.STF`) are found
without renaming them.

**Watcom language extensions.** DOS keywords (`near`/`far`/`huge`, `__interrupt`,
`cdecl`/`pascal`, `#pragma aux` inline assembly, implicit-`int` `const`) have been removed
from all source files or guarded with `#ifdef __WATCOMC__`.

**OS platform guards.** All `#ifdef OS_LINUX`/`OS_DOS`/`OS_MAC` guards have been removed;
Linux code paths are now unconditional. `OS_LINUX` and `OS_DOS` are no longer defined.

**External binary libraries.** Four binary-only DOS libraries were replaced:
- HMI SOS (digital audio): replaced by SDL3_mixer WAV playback in `soundmgr.cpp`
- HMI MIDI: replaced by SDL3_mixer FluidSynth with in-memory HMP→SMF conversion
- RAD/Smacker (FMV): replaced by libsmacker in `smacker.cpp`/`flicsmk.cpp`
- HMI NetNow (LAN play): fully stubbed; replaced by ENet for multiplayer

**Assembly files.** MODEX.ASM, OSGRPH.ASM, PENTIMER.ASM, and VESA.ASM replaced by
C/C++ equivalents in `sdl_backend.cpp` and `asm_stubs.cpp`.

**Struct packing.** Watcom used 1-byte minimum-size enums and no struct padding.
Disk-mapped structs (`ResInfo`, `MapRec`, `MapIdxRec`, `ChunkInfo`) use `#pragma pack(1)`
and `unsigned char` for enum fields to match the on-disk layout.

**`mode_t` conflict.** The game's `enum mode_t` in `fmt_lbm.hpp` was renamed to
`enum lbm_mode_t` to avoid a conflict with the POSIX typedef.

**Anti-piracy code** removed from `bam.cpp` and `world.cpp` (caused crashes on Linux).
