# Asset audit

## Edition presentation

The physical asset files have been updated: GAME/DEMO/30.SMK is the short
206,160-byte version; GAME/30.SMK is the long 4,318,552-byte version. The full
runtime searches its root before STFS. The original short STFS/30.SMK remains
as an unused fallback copy; it is not selected while the root file exists.

## Verified inventory

- The full MAIN map indexes 1,397 resources; CINES indexes 507. These are
  inventory counts, not proof that every entry is exercised by the recovered
  source. Keep both packed archives and their maps/index.
- All 50 movie IDs in the active cinematic table have a corresponding full
  movie file. STFS contains 50 movies and 50 scripts. 15.SMK has no matching
  script; scripts are optional in the player, so that is not a missing asset.
- The demo MAIN map indexes 828 resources. All 828 loose DATA files match
  TOOLS/RESOURCE/STUFF.DAT, with no missing or extra manifest files. They are
  needed to rebuild the demo archive, even though gameplay reads MAIN.STF.
- Full root and STFS copies of 15.SMK, 20.SMK, 20.SCR, and 30.SCR are identical.
  Root copies take precedence; the STFS copies are redundant fallbacks.

## Keep versus cleanup candidates

- Keep numeric resources, campaign movies/scripts, palettes, fonts, map data,
  sound banks, WAV/HMP resources, HMI digital/MIDI drivers, DOS4GW, and runtime
  configuration. Numeric IDs and data-driven selection make absence of a
  literal filename in source an unreliable deletion test.
- Keep DATA as demo content-pipeline source, not as unwanted duplication.
- SNDSET2.EXE/INI, TEST.HMP, TEST.RAW, HMIDET.386, and HMICARDS.386 support the
  legacy sound-configuration tool, rather than ordinary configured gameplay.
  Retain them while sound configuration/restoration is supported.
- GRVDET.EXE, LOADPATS.EXE, and PATCHES.INI support optional Gravis Ultrasound
  hardware. SNDSETUP.BAT calls the first two. They are unnecessary for our
  Sound Blaster/OPL3 settings, but moving them requires adapting that script.
- Demo root 1.MID is not the engine's configured music format (HMP). Root
  1.HMP/BNK files can be shadowed by packed resources. Retain these historical
  standalone audio inputs until their tool uses have been fully traced.
- BAM executables, AUDIO.LOG, memory dumps, saves, and campaign progress are
  generated/local state, not distributable source assets. They are Git-ignored.
  Do not delete user saves as cleanup.

## Restoration still outstanding

No required full cinematic is missing from the active table. Optional old intro
35.SMK is referenced by an Alt-key easter-egg path but is not present; it is not
needed for normal play. Completeness of every campaign scenario has not yet
been tested. Retail loose resource sources are not available in this tree, so
the retail MAIN.STF/CINES.STF cannot currently be regenerated from source here.

Do not restore multiplayer, registration, installer programs, or soundtrack
exports: those are intentionally outside the current single-player scope.
This audit makes no new asset deletions; redundant and optional files are
cleanup candidates, not evidence that their original content was obsolete.
