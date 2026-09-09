# Optional legacy middleware

This directory preserves the disabled integrations and binary-only vendor SDK inputs:

- `source/network` — original Tigre NetNow and Greenleaf serial/modem transports.
- `source/audio` — original HMI SOS sound manager and unused 8-track implementation.
- `source/video` — original RAD Smacker cinematic player.
- `vendor/network` — Greenleaf Comm++ and HMI NetNow headers/libraries.
- `vendor/audio` — HMI SOS headers/libraries.
- `vendor/video` — RAD Smacker headers/library.

The active tree contains API-compatible facades at the original filenames, allowing the game to build without these libraries. Restoring a feature should be done independently behind a build option rather than copying all middleware back at once. None of the vendor library source code is present in this repository.
