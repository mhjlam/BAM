# Legacy build and distribution notes

This document records useful information from the historical scripts and generated files now stored under `Archive`. It is not a promise that every archived binary is safe or useful on a modern system.

## Core build

The primary makefiles were `Source/Tigre/TIGRE.MAK`, `Source/Bam/BAM.MAK`, and `Source/Bam/BAMF.MAK`. Their source manifests and effective compiler/linker settings are represented by `Build/Build.ps1`.

- Tigre produced `TIGRE.LIB`. Its selected display backend was `XMODDISP.CPP` plus `MODEX.ASM`; the alternative SVGA/VESA selection was commented out. `OSGRPH.ASM` and `VESA.ASM` were also assembled.
- The common compiler assumptions were signed `char` (`/j`), disabled stack checks where selected (`/s`), warning level 3, 386 or 486 register-oriented code generation (`/3r` or `/4r`), and Watcom debug information (`/d1` or `/d2`). Assembly used the flat memory model (`-mf`).
- The shipped resource index uses byte-packed structures: its sole `MapIdxRec` is 18 bytes. The restoration build explicitly uses `/zp1`; Open Watcom's wider default padding otherwise makes the index read fail and causes misleading loose-file errors such as `File not found: .\9050.fon`.
- The old Tigre makefile warned not to apply its optimized flags to `RESMGR.CPP` because doing so caused crashes. The compatibility build retains the conservative flags for that file.
- BAM originally linked `TIGRE.LIB`, `SOSDW1CR.LIB`, `SOSMW1CR.LIB`, `NETNOWR.LIB`, `CPFR32.LIB`, and `SMACK.LIB`, requested a link map, and used an 8 KiB stack. The restoration build now links only the rebuilt `TIGRE.LIB` and retains the 8 KiB stack and map.
- `BAM.MAK` had an apparent typo that built `BAMFUNC2.OBJ` from `BAMFUNCS.CPP`; `BAMF.MAK` correctly used `BAMFUNC2.CPP`. The new build follows `BAMF.MAK`.
- The old paths such as `C:\BAM\TIGRE`, `I:\BAM\INTDEMO`, and `D:\BAMDEMO` were workstation-specific and are intentionally not reproduced.

## Middleware source audit

The libraries were inspected with Open Watcom's librarian. Only `TIGRE.LIB` is reproducible from source in this repository.

| Library | Identity | Source present? | Restoration status |
| --- | --- | --- | --- |
| `TIGRE.LIB` | Tachyon's engine | Yes | Rebuilt from `Source/Tigre`. |
| `CPFR32.LIB` | Greenleaf Comm++ 3.00 serial/modem support | No | Archived; replaced by unavailable serial/modem facades. |
| `NETNOWR.LIB` | HMI NetNow IPX/network support | No | Archived; replaced by an unavailable network facade. |
| `SOSDW1CR.LIB` | HMI SOS digital audio | No | Archived; replaced by a no-op sound facade. |
| `SOSMW1CR.LIB` | HMI SOS MIDI | No | Archived; replaced by a no-op music facade. |
| `SMACK.LIB` | RAD Smacker 2.0y playback | No | Archived; cinematics are skipped successfully. |

The binary member paths identify the vendors and products but do not contain reconstructable source. In particular, Open Watcom 1.9 reached the link step with the original libraries, but Greenleaf required old Watcom runtime symbols such as `__wcpp_2_dtor_array_store__`; that confirmed an ABI mismatch rather than missing BAM code.

## TIG_GL, Greenleaf, and Magna Carta

`GL` means **Greenleaf**, not OpenGL. `Source/Tigre/TIG_GL/CHANGES.DOC` records a December 1995 communications migration from Magna Carta to Greenleaf Comm++. Magna Carta and Greenleaf were commercial serial/modem communications libraries; they are unrelated to graphics.

`TIG_GL` is a partial integration snapshot, not a separate renderer or the shipped engine. Most of its headers are byte-identical to the main Tigre tree, while its communications and memory files are older/smaller. The main `Source/Tigre` versions contain the integrated later work. `TIG_GL` and the still earlier `NEW_SRL` snapshot are therefore under `Archive/ObsoleteSource`, along with the unreferenced VESA assembly backend. The active shipped display path remains `XMODDISP.CPP` plus `MODEX.ASM` and `OSGRPH.ASM`.

## DOS boundary

DOS-specific does not automatically mean obsolete for the first milestone. DPMI, DOS/4GW, VGA Mode X, palette access, timer code, keyboard, mouse, and the assembly blitters are required to produce and run the DOS executable in DOSBox. Installer programs, sound-card detection/drivers, registration programs, development scripts, alternate unused renderers, networking transports, and media middleware are outside that boundary and are archived.

## Standalone tools

Each old utility makefile represented a separate executable rather than part of BAM. Their files now live in categorized projects below `Source/Tools`:

| Tool | Recorded inputs and purpose |
| --- | --- |
| `MAKERES` | Tigre resource builder using compression, configuration, RLE/T12, writer, and tool-memory modules. |
| `RESINFO` | Resource/configuration inspection tool. |
| `INSTALL` | DOS installer stub using `INSTALL.CPP` and Tigre's DPMI object. |
| `MAKEMIF` | Map-information generator using `MAKEMIF.CPP`, `WRITERES.CPP`, and Tigre. |
| `MAKESERL` | Serial-data utility built from `MAKESERL.CPP`. |
| `MAKEWPIF` | Windows 95 PIF-generation utility. |
| `MAPEDIT` | Map editor using game map/unit/dialog code, Tigre, SOS audio, NetNow, and a historical communications library. |
| `SHOWXFLI` | FLIC viewer; its makefile names `FLIC.CPP`, which is absent from this checkout, so the target is incomplete. |
| `TIL2TLB` | Tile-to-tile-library converter using `WRITERES.CPP` and Tigre. |
| `TXT2SQB` | Text-to-squib converter using `WRITERES.CPP` and Tigre. |

The old `M.BAT` deleted `TIGRE.LIB`, enabled SMARTDrive write-behind caching, ran `wmake -e -f tigre.mak`, captured output in `ERR`, then displayed the log. `BKP.BAT` ZIPped headers, source, makefiles, libraries, configuration, response, batch, and assembly files. `TEST.BAT` merely timed `M.BAT`. These behaviors are obsolete and potentially unsafe, so they remain archived.

## Asset pipeline

The 4DOS `.BTM` scripts describe how the demo's monolithic resource archive was assembled:

- `GETNAMES.BTM` enumerated `.ANI`, `.HMP`, `.WAV`, `.PAL`, `.FON`, `.SQB`, `.BNK`, `.TLB`, and `.MIF` files.
- `GATHER.BTM` appended short names to `STUFF.DAT` and absolute source names to `FILES.TXT`.
- `GETFILES.BTM` copied those absolute paths into a staging directory and invoked `MAKESTUF.EXE`. It notes that Smacker `.SMK` movies and `.SCR` scripts could not be included in STF archives and therefore remained loose files.
- `STUFF.DAT` described an archive named `main`; its resulting files were `MAIN.STF`, `MAIN.MAP`, and `MAP.IDX`.
- `MOVEFILE.BAT` distributed those outputs, copied source between development machines, and reset archive attributes. Its absolute drive mappings have no meaning in the current repository.

`INTDEMO.TXT`, `FILES.TXT`, `FILES.BKP`, and `STUFF.DAT` are generated manifests. The active resource files remain under `Game`; the manifests and their generator executables are archived.

## Installer and DOS sound support

The DOS and Windows 95 installer configurations required a 486 and about 27 MB of disk space, installed into `C:\INTRPLAY\BAMDEMO`, ran sound setup, optionally created a Windows 95 PIF, displayed final messages, and deleted temporary installer helpers.

The archived sound setup supported Sound Blaster variants, Gravis UltraSound, Ensoniq SoundScape, Microsoft Sound System, Pro Audio Spectrum 16, ESS AudioDrive, Roland devices, MPU-401, and several other period-specific cards. `SNDSETUP.BAT` detected Gravis hardware, loaded patches when found, and then launched `SNDSET2.EXE`. `PATCHES.INI` contained the UltraSound General MIDI patch-loading and fallback map.

The active `Game/Sound.cfg` selects no digital or MIDI device. The `.386` drivers, detection/setup executables, test audio, registration program, installers, PIF helper, and messages are archived. `Game/Dos4gw.exe` remains active because the DOS executable requires the extender.

The demo also checked `HMICARDS.386` as an integrity marker and armed a delayed memory-corruption crash when the file was missing or changed. That check has been removed from the active game because the HMI audio layer is disabled; the original implementation and driver remain preserved in `Archive`.

## Cinematics

The loose demo cinematics and scripts are preserved in `Archive/OptionalMiddleware/RuntimeMedia`. The archived RAD library is a valid Watcom 32-bit DOS library for Smacker 2.0y. The original `FLICSMK.CPP` wrapper compiles with Open Watcom 1.9 and links into the current game without the old SOS audio libraries.

An isolated DOSBox test confirmed that this combination opens, decodes, and displays the silent `15.SMK` Interplay logo. It then fails reproducibly on the audio-bearing `20.SMK` with a divide-by-zero inside the library's proprietary `unsmack.ASM` code. Suppressing the wrapper's audio requests does not prevent the fault. FFmpeg successfully decodes all three archived movies, so the evidence points to the old binary decoder rather than damaged media.

The original library is therefore retained as archaeological material, but is not enabled in the normal build. The active middleware-free build advances past unavailable cinematics—including the three-part startup sequence—instead of showing the historical “insert CD” retry dialog. For a future native port, [libsmacker](https://libsmacker.sourceforge.net/) is the smallest focused replacement; [FFmpeg's Smacker decoder](https://ffmpeg.org/doxygen/7.0/libavcodec_2smacker_8c.html) is a broader alternative.
