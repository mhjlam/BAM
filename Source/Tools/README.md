# Legacy standalone tools

These sources are not linked into the Tigre engine or BAM executable. They are grouped by role and exposed as Utility projects in `Bam.sln`; the projects do not build by default because several targets need additional restoration work.

- `Installer` contains the DOS installer and PIF/message helpers.
- `ResourceTools` contains resource, map-info, serialization, tile-library, and squib builders.
- `MapEditor` contains the original map editor and its tool-specific tile library implementation.
- `MediaTools` contains the incomplete historical FLIC viewer.
- `MiscTools` contains small text/development utilities without surviving makefiles.

The original makefiles remain in `Archive/BuildScripts`, and their useful build details are summarized in `Docs/LegacyNotes.md`.
