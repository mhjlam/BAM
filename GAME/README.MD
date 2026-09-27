# Full game assets

These files were copied from the local full Blood & Magic installation. Full is the default edition. Demo assets and their loose resource inputs are kept separately under GAME/DEMO.

Retail assets and runtime binaries in this folder are local-only and ignored by
Git. They are not included in the repository. Only the demo assets, this note,
and the full-edition resource/sound configuration are tracked. A fresh clone
can run the demo with `-Edition Demo`; full play requires your own retail files.

Build and launch with:

```powershell
.\BUILD\BUILD.PS1 -Edition Full -Configuration Release -Test
```

The essential packed resource set is MAIN.STF, MAIN.MAP, MAP.IDX, CINES.MAP, and STFS/CINES.STF. MAP.IDX lists both the main and cinematic archives. RES_CFG.HPP uses local paths instead of the original F: CD drive and prefers the runtime root before STFS for loose cinematic files. DOS4GW.EXE is also required to run the rebuilt executable.

The demo uses the short 30.SMK intro; the full edition uses the long root-level 30.SMK. This is our chosen edition presentation.

Loose SMK movies and SCR scripts, sound banks, HMI drivers, and sound setup/configuration files are retained. Both editions now use Sound Blaster digital audio and OPL3 music, with the original SOS manager and Smacker player enabled. See DOCS/ASSET_AUDIT.MD for runtime, pipeline, and optional-file distinctions.

Existing saves, player/campaign progress files, installers, registration programs, and original executable binaries were not imported. This source tree is recovered from the demo; compiling its full-game code paths does not establish complete compatibility with every retail scenario. Full-game assets remain subject to their original license.
