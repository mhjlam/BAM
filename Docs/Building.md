# Building Blood & Magic

The checked-in source is a 32-bit DOS codebase written for Watcom C/C++. The Visual Studio solution uses Visual Studio as the IDE and build coordinator while retaining a Watcom compiler and linker. Compiling the original source directly with MSVC would require a port rather than a faithful first build.

## Toolchain baseline

The compatibility target is **Watcom C/C++ 10.x**, targeting **DOS/4GW (32-bit x86)**. Several bundled headers explicitly say they were adapted for Watcom C++ 10.0 in August 1994, and the makefiles use that compiler's command line, memory model, object librarian, and linker conventions.

The preferred toolchains, in order, are:

1. Watcom C/C++ 10.0 for the closest reproduction of the original development environment. No official downloadable 10.0 package has been identified; use legitimately owned original media if available.
2. [Watcom C/C++ 11.0c](https://github.com/open-watcom/open-watcom-1.9/releases/tag/w11.0c-zips) as the closest readily available archived commercial release.
3. [Open Watcom 1.9](https://github.com/open-watcom/open-watcom-1.9/releases/tag/ow1.9) as the conservative open-source fallback with a Windows installer.

Open Watcom v2 can be tried later, but it is a moving next-generation branch and is better treated as a compatibility step after the first faithful build. The build driver deliberately uses only the original Watcom-family tools and flags, so all of the choices above expose the same interface.

The repository currently has Open Watcom 1.9 extracted under `Tools/open-watcom-1.9`. The PowerShell build detects that location and initializes its process environment automatically. The toolchain is local and ignored by Git; see `Tools/README.md` for its source and checksum.

If the local installation is absent, initialize another Watcom environment before starting Visual Studio. The following commands must be discoverable on `PATH`:

```text
wcl386.exe
wasm.exe
wlib.exe
wlink.exe
```

The exact environment command depends on the Watcom package. A typical installation provides an environment batch file, or requires `WATCOM`, `PATH`, `EDPATH`, and `INCLUDE` to be initialized. Start Visual Studio from that initialized command prompt so its Makefile projects inherit the variables.

## Solution layout

- `Bam.sln` — Visual Studio 2022 solution.
- `Source/Tigre` — engine source; compiled into an intermediate `Tigre.lib`.
- `Source/Bam/Bam.vcxproj` — the one buildable project, containing both the game and engine source views.
- `Build/Build.ps1` — unified out-of-tree Watcom 10.x-compatible build. Every invocation builds Tigre first when needed and then links Bam.
- `Build/Out/<project>/<configuration>` — generated objects and binaries.
- `Game` — active game assets and runtime resource metadata, including `Data`.
- `Archive` — preserved installers, original executable, hardware support, historical build material, and generated manifests not needed by the active build.

Both solution configurations target DOS even though Visual Studio labels the host platform `Win32`. `Debug` retains Watcom debug information; `Release` follows the more optimized `BAMF.MAK` intent. Generated files never overwrite the historical binaries and libraries checked into the repository.

Build from Visual Studio with **Build Solution**, or from an initialized PowerShell prompt:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\Build\Build.ps1 -Configuration Debug
```

The restoration configuration builds without proprietary middleware. Network/serial transports, HMI SOS audio, and RAD Smacker playback have small API-compatible disabled implementations. Their original sources, headers, and libraries are preserved under `Archive/OptionalMiddleware`. This keeps the initial target focused on single-player game logic, graphics, keyboard, and mouse input.

The following command has been verified to rebuild both `Tigre.lib` and a DOS/4G `Bam.exe` with Open Watcom 1.9:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\Build\Build.ps1 -Configuration Debug -Rebuild
```

The executable is written to `Build/Out/Bam/Debug/Bam.exe`. Compiler warnings from this pre-standard C++ code are currently suppressed in the successful build output; a failing tool prints its complete diagnostics.

## Test target

To build and immediately run Bam in DOSBox without mounting or navigating manually:

```powershell
.\Build\Build.ps1 -Configuration Release -Test
```

The same operation is exposed as the `Test` target of `Source/Bam/Bam.vcxproj`. From a Visual Studio developer prompt, it can be invoked with:

```powershell
msbuild .\Source\Bam\Bam.vcxproj /t:Test /p:Configuration=Release /p:Platform=Win32
```

The launcher mounts the selected `Build/Out/Bam/<Configuration>` directory as `C:`, runs `Bam.exe`, and exits DOSBox when the game closes. DOSBox must be available on `PATH`.

## Running in DOSBox

`Game/Dos4gw.exe` is intentionally active because the rebuilt executable needs the DOS/4GW extender at runtime. The build stages it and the packed runtime resource set (`Main.stf`, `Main.map`, `Map.idx`, `ResCfg.hpp`, and `Sound.cfg`) beside each `Bam.exe`. You can therefore launch directly from the selected output directory:

```text
mount c D:\github\games\BAM
c:
cd Build\Out\Bam\Debug
Bam.exe
```

The raw files under `Game/Data` are retained source assets; Bam normally loads their packed copies through `Main.map` and `Main.stf`. Numeric filenames such as `9050.FON` are stable resource IDs and intentionally retain their original DOS spelling.

The `9050.FON` startup error means Bam did not open the packed resource set. Resource 9050 is the default interface font, already present in `Main.stf` and as `Game/Data/9050.FON`; it does not need to be recreated. Run the staged executable above rather than an unstaged copy from another working directory.

The resource-map structures must use Watcom byte packing (`/zp1`). Without it, `Map.idx` cannot be read and the same misleading loose-font fallback occurs even when all runtime files are present. Fatal errors now also write `Panic.log` beside the executable so DOSBox startup failures remain visible after graphics mode closes.

The Release build has been smoke-tested in DOSBox: it remained running after startup and produced no `Panic.log`. Sound, movies, and multiplayer are deliberately unavailable in this configuration; movie rooms immediately advance to their next room.

## Historical makefiles

The `.MAK` files are preserved under `Archive/BuildScripts` at paths mirroring their former source locations. Their source is organized into non-building Visual Studio utility projects beneath `Source/Tools`: Installer, ResourceTools, MapEditor, MediaTools, and MiscTools. These targets are independent of the game and several still depend on historical components, so they are excluded from Build Solution. See `Docs/LegacyNotes.md` for a condensed record of their useful information.
