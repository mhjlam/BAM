## Save System

### Why a new save system was needed

The DOS version used TIGRE's `MemMgr` grip-dump approach: it serialized the entire memory manager state — all handles and their raw contents — as a flat binary blob. This worked on DOS because:

- 32-bit pointers round-tripped safely (same process layout every run)
- The grip table was small enough to dump wholesale
- `size_t` and pointer-derived sizes were consistent

On 64-bit Linux none of those properties hold. Pointers are 8 bytes, grip numbers are reassigned after every `ClearAllocations()` call, and C++ vtables and raw pointers cannot be round-tripped across process instances. The Linux port therefore replaces the grip-dump with an explicit field-by-field JSON serializer.

---

### Key source files

| File | Role |
|------|------|
| `CODE/TIGRE/jstream.hpp` | `JsonStream` — bidirectional JSON read/write primitive |
| `CODE/TIGRE/savestream.hpp` | `SaveStream` — legacy binary stream (retained but unused by live code) |
| `CODE/TIGRE/savemgr.hpp/.cpp` | `SaveMgr` — file I/O, callback dispatch, file enumeration |
| `CODE/TIGRE/savebase.hpp/.cpp` | `AtSave`/`RemoveAtSave` — callback registration (LIFO) |
| `CODE/SRC/bam.cpp` | `GlobalSave()` — the sole registered `AtSave` callback |
| `CODE/SRC/world.cpp` | `World::Save()` — drives all child subsystem serialization |
| `CODE/SRC/units.cpp` | `Unit::Save()` |
| `CODE/SRC/maps.cpp` | `Map::Save()`, `MapSpace::Save()` |
| `CODE/SRC/viewport.cpp` | `BAM_Ani::Save()`, `ViewPort::Save()` |
| `CODE/SRC/unitlib.cpp` | `UnitLib::Save()` |
| `CODE/SRC/items.cpp` | `ItemMgr::Save()`, `BAMItem::Save()` |

---

### JsonStream — the core primitive

`JsonStream` (`jstream.hpp`) wraps a `nlohmann::json` node and a `saving` bool. Every `sync()` overload writes a named key when saving and reads it back (with a safe default) when restoring. Because the same call works in both directions, there is no separate paired write/read block to keep in sync.

```cpp
JsonStream js{j["subsystem"], true};   // saving
JsonStream js{j["subsystem"], false};  // restoring
js.sync(myInt32, "myInt32");
js.syncGrip(myGrip, "myGrip");        // grip saved as uint32 index; generation discarded
js.syncBytes(buf, n, "myBuf");        // raw POD block, stored as base64 string
js.syncStr(charArray, len, "myStr");  // NUL-terminated char array, stored as JSON string
js.syncEnum(myEnum, "myEnum");        // enum cast to int32
js.syncArray(myArr, "myArr");         // typed 1-D or 2-D arrays
js.syncRect(myRect, "myRect");        // Rectangle (x1/y1/x2/y2 by name, no vtable)
js.sentinel("tag");                   // no-op in JSON; retained for call-site compatibility
```

Grips are saved as `uint32_t` index only (generation is runtime-only). As with the old binary format, saved grip values are never used directly after restore — all inter-object references go through serial numbers.

`syncBytes` encodes raw POD data as base64 string. `syncArray` stores typed arrays as JSON arrays of numbers. `syncRect` saves `Rectangle` fields by name to avoid capturing the vtable pointer.

---

### File format

Each save slot uses **two files** stored in the XDG preference directory (`get_pref_dir()` in `file.cpp`):

```
{pref_dir}/{saveNum}.sav  — plain-text header (human-readable, used by UI)
{pref_dir}/{saveNum}.dat  — plain JSON data (human-readable, editable)
```

The `.sav` header:
```
BAMJ 2\n
{save name}\n
```

The `.dat` file is a pretty-printed JSON object (`root.dump(2)`). Its top-level structure:
```json
{
  "global": { ... },     // bGlobal scalars and BAM_Application fields
  "randGen":  { ... },   // RNG state
  "randGen2": { ... },
  "world": {
    "spaces":  [ ... ],  // one entry per MapSpace
    "units":   { "0": [...], "1": [...], ... },  // indexed by side
    "items":   [ ... ],
    "bgAnis":  [ ... ],
    ...
  }
}
```

Both files are written atomically: `SaveMgr::Save()` writes each to a `.tmp` sibling and calls `rename()` to make the swap visible. Version: `BAMJ 2` (format version 2, declared in `savemgr.cpp`).

File enumeration for the save/load menu uses `opendir`/`readdir` on `get_pref_dir()`. `SaveMgr::GetFirstSave()` / `GetNextSave()` return any file whose name ends in `.sav`.

---

### Callback system

`AtSave(fn)` registers a callback `bool fn(uint16 state, nlohmann::json& root)`. Callbacks are stored in a fixed array of 32 entries and called in LIFO order. Only one callback is registered in the current port — `GlobalSave` in `bam.cpp` — registered at game startup via `AtSave(GlobalSave)`.

The six save states are:

```
BEFORE_SAVE   DURING_SAVE   AFTER_SAVE
BEFORE_RESTORE  DURING_RESTORE  AFTER_RESTORE
```

`SaveMgr::Save()` calls `ExecuteAtSaveFunctions` for BEFORE, DURING, and AFTER in sequence; `SaveMgr::Restore()` does the same for the restore states, after parsing the `.dat` JSON.

---

### Save sequence (DURING\_SAVE)

`GlobalSave(DURING_SAVE, root)` serializes everything into the JSON tree in a fixed order:

**Block 1 — `root["global"]`** (`bam.cpp:GlobalSave`)

Uses a `JsonStream js{root["global"], true}` for:
- `BAM_RoomMgr`: `curRoomNum`, `newRoom`, `newRoomMode`, `prevRoomNum`, `prevRoomMode`
- Story / progression: `storyLine`, `legendStart`, `missionsDone`, `curPath`, `prevChooseSide`
- Version: `versionNum`, `versionSubNum`, `buildID`
- Room mode: `roomMode`, `replayMap`, `aiUnitMultiplier`, `aiOveride`
- Character creation: player and enemy character fields, name arrays, last-used arrays
- Net state, score/XP arrays, alignment counters
- Misc: `cinematic`, `chooseSide`, `writeOut`
- Research flags, tutorial flags, alt music number

**Block 1 continued — `BAM_Application` fields** (inlined in `GlobalSave`)
- Gameplay settings, side colors, stats arrays, player types, voice chains, language

**RNG state — `root["randGen"]`, `root["randGen2"]`**
- Written via `bGlobal.randGen.Save()`

**Block 2 — `root["world"]`** (`world.cpp:World::Save(DURING_SAVE, root["world"])`)

Uses a `JsonStream js{j, true}` for world scalars:
- `currFPS`, `framesRun`, `dragSuspendCnt`, `aiOn`, `mapResNum`, `tileResNum`, `fIsPaused`, `fPauseViaNet`
- `PauseFlashTimer`, `lastUnitSerialDrawn`
- `mana[]`, `lastMana[]`, `lastManaCel`
- CLUT buffer contents for all sides (the colour data, not the grip — saved via `syncBytes`)
- `playerSide`, tick timers
- Action pool scalars (gTargets written as zeros; re-resolved at runtime)
- `soundPos[]`, `musicNum`
- `winCons[]`, `tWorldEnds`, `worldEnder`
- Unit/structure statistics arrays
- `nextSerialNum`; `serialNums[][]` are rebuilt on restore, not saved
- Network packet state (stubbed)
- `currFrame`, stat/research fields

**Object graph** (still in `World::Save`):

1. `map.Save()` — swap-series state into `j["map"]`
2. `j["spaces"]`: one entry per MapSpace — `h` header (HP, owner, func, coords), `defenders[]`, `lastAttackerSerial`, `fFoundationFilled`, `size`, `serialNum`, `tiles[][]`, `oldTileNums[][]`, `newBldgType`
3. `unitLib.Save()` — `fEnemyFlags[][]` into `j["unitLib"]`
4. `j["units"]`: for each side, for each unit — `Unit::Save()` writes position, stats, all gameplay state, AI data, serial number, path array
5. `j["items"]`: `itemMgr.Save()` + per-item `BAMItem::Save()` (item type + `tileX`/`tileY`)
6. `j["bgAnis"]`: background animations (portals, `#ANIM` scene entries) — resType, resNum, cel, tileX/Y, animation flags, delay, priority, CLUT side index
7. `vPort.Save()` — `fog[][]`, `fogTile`, camera (ViewX/Y, CursorX/Y), verb/targeting state, misc flags

---

### Restore sequence (DURING\_RESTORE)

The restore path runs the same `JsonStream` sync calls with `saving=false`, interleaved with object creation:

**`BEFORE_RESTORE`** (in `GlobalSave`):

- Pins `BAM_Application` with `NO_PURGE_GRIP` so it survives `ClearAllocations()`
- Zeroes the three list members of `pBam` (roster, serviceables, receivers)

**`DURING_RESTORE`** — Block 1: reads `bGlobal` scalars and `BAM_Application` fields from `root["global"]`.

After the block, World restore begins:

```
new World              // creates gSelf, gPal, viewport, HUD skeleton
pWorld->Save(DURING_RESTORE, root["world"])
  -> reads World scalars from j
  -> map.Load(mapResNum)       // recreates gSpaces[], building serial numbers, lBuildings
  -> map.FixupClusters()
  -> unitLib.Load()            // unit stat groups
  -> vPort.Setup(gSelf)        // viewport grips, context registration
  -> WorldMap::Setup()         // shadow cursor, master viewport cel
  -> nextSerialNum = savedNextSerialNum  // override map.Load()'s counter
```

Then the object graph is read back:

- `map.Save(DURING_RESTORE)` overlays swap-series state on top of `map.Load()`'s base state
- `MapSpace::Save(DURING_RESTORE)` overlays HP/owner/func changes, defender lists, tile mutations
- Units: for each entry in `j["units"][side]`, `new Unit` is created, `Save(DURING_RESTORE)` fills its fields, it is moved to the correct side list, serial number registered, `SetType()` rebuilds type-specific data, `PlaceUnitGrip()` places it in the world, `RunAnimation(ST_ANIM_GUARD)` starts guard stance
- Items: `NewItem(type)` creates each item, `Save(DURING_RESTORE)` reads tile position, item is placed and registered
- Background animations: each recreated via `vPort.NewAni()` with saved parameters
- `vPort.Save(DURING_RESTORE)` reads fog-of-war, camera position, verb state

**`AFTER_RESTORE`** (in `World::Save`):

- Reloads `pTileLib` from the resource
- Restores the HUD: loads scenario palette, repaints interface screen, recreates `PalCycler` for selection-square flash, calls `SetupVerbButtons()` and `SetupMainButtons()`
- Fixes up CLUT pointers for all animated objects and viewport cursor
- Re-derives `pCurrPool`/`pAltPool` from the restored `fPoolFlip`
- Calls `ai.Setup(side)` for each computer-controlled side
- Calls `vPort.MoveView()` and `WorldMap::MoveCursor/MoveShadowCursor()` to sync minimap, then `WorldMap::Draw()` to repaint the minimap fog buffer

**`AFTER_RESTORE`** (in `GlobalSave`):

- Re-anchors `pBam` from the protected grip
- Resets `squib1.gRes` to prevent stale-grip panic on next squib load
- Clears `bGlobal.gSnap` (debug screen-capture, not saved)

---

### Design decisions

**No grip serialization.** Grip numbers are memory-manager handles that change after every `ClearAllocations()`. All inter-object references that must survive save/load are stored as serial numbers (`primaryTargetSerial`, `secondaryTargetSerial`, `packLeaderSerial`, `mountSerial`, etc.) and re-resolved at runtime.

**Recreate, don't restore the object graph.** Rather than reconstructing the full TIGRE heap state, the restore path tears everything down (`ClearAllocations()`) and rebuilds the object graph from scratch — `new World`, `map.Load()`, `new Unit`, `NewItem()`. Saved data is overlaid on top of this freshly constructed state.

**`map.Load()` on restore.** The map is large and mostly static. Instead of serializing it, the restore path calls `map.Load(mapResNum)` to reproduce the base state, then `MapSpace::Save(DURING_RESTORE)` overlays only the runtime-changed fields (HP, owner, swap-series state, captures).

**Transient fields are reset, not saved.** UI grips (`gGroupCursor`, `gTerrainAni`, `gAuxAni`), animation grips (`gMasterCel`), and hover state are explicitly zeroed on restore and re-established by normal game startup paths.

**CLUT content, not CLUT grips.** The `clut[side]` grips point to colour-lookup buffers created by `new World`. On restore, new grips are assigned to new buffers; the old grip numbers are useless. Instead, the content of each CLUT buffer is saved with `syncBytes(ADeref(clut[i]), CLUT_SIZE, key)` and read back into the new buffers.

**Action pool `gTargets` are skipped.** The action pool's target grip arrays are written as zeros on save and skipped on restore. Target references are re-established at runtime via the serial-number mechanism.

**JSON is human-readable and editable.** The `.dat` file is pretty-printed with 2-space indentation. Binary blobs (CLUT buffers, fog arrays) are stored as base64 strings. This makes it straightforward to inspect or patch save files without a hex editor.

**Atomic writes.** `SaveMgr::Save()` writes each file to a `.tmp` sibling first, then calls `rename()`. A crash mid-write leaves the previous save intact.

---

### Known limitations

- **AI personality values** (`eAggression`, `eWits`, `eCaution`, `eTerrainIQ`, `eItemIQ`, `eSpellIQ`) are not saved. After loading, they revert to constructor defaults (all 99). These values are normally set by `ProcessSceneConfig()` which is not re-run on load. See `known_bugs.txt`. `AI::Setup()` is still called in `AFTER_RESTORE` to re-initialize hotspot counts and portal coordinates.

- **Pack follower lists** (`glFollowers`), the Brigand's backpack (`glBackpack`), and auxiliary animations (`gTerrainAni`, `gAuxAni`) are reset to 0 on restore and re-derived on the first game cycle.

- **`BAM_Application::Save()`** (the original linker-trick version using `&bamAppDataStart`/`&bamAppDataEnd` boundary symbols) is not used by the Linux port. Fields are synced individually inside `GlobalSave()`.
