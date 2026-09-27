# Blood & Magic 1.01 reconstruction

This repository tracks the confirmed non-network changes from the official Blood & Magic 1.01 patch in source form. The reference files are kept outside version control.

## Reference executables

The retail CD uses `.DAT` extensions for its language-specific executable payloads. During installation, the selected payload is copied to `BAMMAIN.EXE`:

- `BAM.DAT`: English executable, version 1.00
- `BAMD.DAT`: German executable, version 1.00
- `BAMF.DAT`: French executable, version 1.00

The official patch replaces the installed `BAMMAIN.EXE`; it does not supply a new `BAM.DAT` or modify game resources.

The binaries used for comparison were:

| Binary | Size | SHA-256 |
| --- | ---: | --- |
| Official interactive-demo `BAM.EXE` (0.93) | 1,006,113 | `23690EBC14D6F2787A3440E14458070213170883DD8FC8AE4E2EFCF419396FCE` |
| English retail `BAM.DAT` (1.00) | 1,225,319 | `EF0A56A529D28A5A24CBE911FC134CBE473BDEC4E498415CF90FC5AD392078A4` |
| Official patch `BAMMAIN.EXE` (1.01) | 1,234,535 | `0C54CFBBF6493FA9DD2D64E56E0099B5598FD63EBA2BF62228BBB1DCE62D9B5F` |

The embedded build dates are October 8, 1996, November 14, 1996, and January 3, 1997 respectively.

## Source-version audit

The `0.93` values formerly in `BamGlobal` do not reliably date the recovered source tree. The tree contains both product configurations: defining `INTERACTIVE_DEMO` selects the restricted demo, while omitting it selects the complete retail campaign. The full-build branches already contain the retail story and campaign progression, all unit types, save/load support, full menu and death/assessment flows, and writable preferences. The recovered `Unit::CycleMoveTo` also already contained the exact January 1997 Brigand repair which is absent from the November 1996 retail executable. The source is therefore a mixed/later snapshot which retained stale demo version metadata, not simply the October 1996 demo source.

The 0.93 demo and 1.00 retail executables were also built differently. Several large game modules in the demo makefile use debug code generation, while the retail makefile optimizes every module. That accounts for extensive instruction-layout and diagnostic-string differences which are not source changes. A source-literal audit found 218 literals present only in the debug-built demo and no retail-only literal missing from the demo/source set. Direct function matching likewise identifies hundreds of unchanged routines after relocation. The remaining large, expected delta is produced by the existing `INTERACTIVE_DEMO` branches and the additional retail-linked code.

No additional 0.93-to-1.00 gameplay change could be established which is absent from this source tree. Accordingly, no speculative binary-derived changes were added. The full build already compiles the retail paths by defining `BAM_FULL_GAME`, which prevents `INTERACTIVE_DEMO` from being defined.

## Confirmed non-network changes

Binary comparison confirms the following source-level changes:

1. The internal version pair changes from 1,0 to 1,1, denoting releases 1.00 and 1.01. The recovered source retained the demo's 0,93 pair, so `BamGlobal` now reports the 1.01 pair. The full-game menu now zero-pads the patch component and displays `1.01`; the demo retains its original `DEMO ver.` label.
2. `Unit::CycleGuardMode` allows an Acolyte to receive meditation mana only below half of `MANA_ACOLYTE_MAX` (10 rather than 20). This is an undocumented 1.01 gameplay change.
3. `Unit::CycleMoveTo` recalculates `stepToXOff` and `stepToYOff` from the unit's current direction before starting each tile-to-tile movement. This is the documented Brigand save-game fix. The recovered source already contained these statements, despite identifying itself as version 0.93, so no new behavioral code was needed for this fix.

Version 1.00 saves are deliberately rejected by 1.01. Save files contain raw object layouts and the original executable also changed its version fields, so accepting an older save would be unsafe.

## Brigand save-game fix

The patch notes describe a problem on Level 3 while playing Red/Aleric: attacking the Brigand after saving should no longer cause a problem. The recovered source spells the character's name `AELRIC`.

### Scenario and creature identification

`GetScenario` reverses each three-map story for Red and then adds 5 to select the Red variant. Consequently, Storyline 1 Level 3 as Red is resource **9115**:

```text
Storyline 1 base maps: 9110, 9130, 9150
Red order:             9150, 9130, 9110
Red variant suffix:       +5,   +5,   +5
Red Level 3:                         9115
```

`BRIGAND` is a creature type, not a named trigger. It is a non-violent thief which seeks items, steals a victim's held item, and stores extra items in its `glBackpack` list. When another unit damages a Brigand, `Unit::TakeAHit` may make it drop an item and, when it was guarding, flee toward a random destination. This puts the creature through the ordinary movement state machine.

The retail installation does not include the loose `9115.MIF` content-pipeline input, and the demo's loose maps are different scenarios. The scenario-specific evidence available here therefore establishes which resource is selected and why a Brigand exercises the affected code, but it does not reconstruct the retail map author's exact placement or event sequence.

### Exact binary change

The relevant `Unit::CycleMoveTo` routine was matched between the 1.00 and 1.01 executables. Immediately before the call which starts movement to the edge of the current tile, 1.01 adds two direction switches equivalent to:

```cpp
switch(currDir)
{
    case DOWN:
    case UP:         stepToXOff = TILE_HORZ_CENTER; break;
    case RIGHT:
    case UP_RIGHT:
    case DOWN_RIGHT: stepToXOff = TILE_HORZ_RIGHT;  break;
    case LEFT:
    case UP_LEFT:
    case DOWN_LEFT:  stepToXOff = TILE_HORZ_LEFT;   break;
}

switch(currDir)
{
    case UP:
    case UP_LEFT:
    case UP_RIGHT:   stepToYOff = TILE_VERT_TOP;    break;
    case LEFT:
    case RIGHT:      stepToYOff = TILE_VERT_CENTER; break;
    case DOWN:
    case DOWN_LEFT:
    case DOWN_RIGHT: stepToYOff = TILE_VERT_BOTTOM; break;
}
```

The emitted constants in 1.01 are `0`, `9`, and `19` for the horizontal offsets and `0`, `18`, and `37` for the vertical offsets, exactly matching the recovered source's tile constants and switches. These assignments are absent from the 1.00 executable.

### Why saving matters

`stepToXOff` and `stepToYOff` are members of `Unit` between `unitDataStart` and `unitDataEnd`. `Unit::Save` writes that entire byte range verbatim and reads it back verbatim on restore. In 1.00, `CycleMoveTo` consumed these fields without assigning them at that point, so a restored moving unit could retain invalid or stale edge coordinates. Attacking the Brigand runs its hit/drop/flee behavior and interrupts or resumes its movement animation, exposing the bad movement state. Version 1.01 makes the state self-consistent by deriving both offsets from `currDir` immediately before use.

The following nearby routines are instruction-equivalent between 1.00 and 1.01 and are not the fix:

- `Unit::Save`
- `Unit::DropItem`
- `Unit::TakeAHit`
- `Unit::CheckPrimaryAction`
- `Unit::CycleAttack`
- `AI::HitUnit`
- TIGRE memory and save-manager routines

Both retail binaries also contain safeguards preventing a Brigand from acting as a conventional attacker. Those safeguards concern the Brigand attacking; the patch note concerns a Brigand being attacked, so they are unrelated.

## Excluded changes

Most of the executable growth in 1.01 belongs to modem/direct-link communications. Multiplayer restoration is outside the scope of the DOS reconstruction, so those changes are intentionally not reproduced.
