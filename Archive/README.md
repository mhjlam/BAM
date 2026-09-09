# Historical archive

Nothing in this directory is required by the current source-only workspace or by `Build/Build.ps1`. The files were moved here rather than deleted so that original behavior can still be investigated and the 1996 distribution can be reconstructed.

## Layout

- `BuildScripts` preserves the original Watcom makefiles and DOS/4DOS helper scripts at paths mirroring the old source tree.
- `BuildArtifacts` contains compiler listings, error logs, response files, backup files, a previously built `TIGRE.LIB`, and a duplicate middleware library.
- `SourceSnapshots` contains ZIP snapshots whose extracted source already exists in `Source/Tigre`.
- `ObsoleteSource` contains duplicate branches, superseded implementations, experiments, and source files omitted by the shipped manifests.
- `OptionalMiddleware` preserves the original network, serial, audio, and Smacker integrations plus their binary-only vendor SDK inputs. Disabled facades at the original active paths allow a middleware-free single-player build.
- `OriginalDistribution/GAME` contains the original game executable, DOS extender, installers, registration and sound-setup programs, hardware drivers, generated asset manifests, and assets already stored beneath a directory named `REMOVED`.

To reconstruct the original demo layout, copy the contents of `OriginalDistribution/GAME` back into `Game`, preserving its `DATA` subdirectory. Do not flatten the directory structure. `Game/Dos4gw.exe` remains active because it is required to launch the rebuilt DOS/4G executable.

See `Docs/LegacyNotes.md` for the relevant information recovered from these files.
