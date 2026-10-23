# Blueprint World Bridge

An experimental Satisfactory Mod Loader (SML) C++ mod that lets one factory blueprint be
placed in saves whose mods use different persistent world names.

Every blueprint records the name of the world it was saved in. Known examples:

| World family | Blueprint world name |
|---|---|
| Vanilla and SatisfactoryPlus | `Persistent_Level` |
| Platform Expansion Program and Skyblock | `SkyblockLevel` |
| Random World Generator | `ProceduralWorldLevel` |
| Empty World Blank Map Level / Blank Map Omni World | `BlankLevel2` |
| PVP - North Vs South | `BlankLevel2_NS` |
| HexFactory | `HexWorld` |
| Aquila Maps and Voxel World | `1_Snowy`, `2_Mountain`, `3_Desert`, `4_Canyon`, `5_Afga`, `6_Afgb`, `7_Voxel` |
| NogsLevel \* | `NogsLevel` |
| Almine2's Map \* | `Almine2MapLevel` |
| Flat Water World \* | `FlatWaterWorldLevel` |

The map-mod names were read from each mod's package on ficsit.app on 2026-10-23. This list is
possibly incomplete or out of date: only mods whose name or description suggested a map were
checked, and mods marked \* target an older game build. SatisfactoryPlus also ships a
`SatifactoryPlusWorld` level, but SF+ saves seen so far use `Persistent_Level`.

The mod has no list of names: it works with any world, including ones not in this table.

## Approach

When FactoryGame loads a blueprint it asks `UFGSaveSystem::FindNewBlueprintWorldMapName` to
translate the blueprint's stored world name. The mod hooks that function:

1. FactoryGame's own lookup (and any other mod's hook) runs first. If a blueprint redirect
   already exists for the stored name, it is used unchanged.
2. Otherwise, if the stored name differs from the current world (ignoring case), the mod
   answers with the current world's name.

The current world is cached from `FWorldDelegates::OnPostWorldInitialization`, which runs before
any actor's `BeginPlay`. Worlds in transient `/Temp/` packages are ignored so a staging world
cannot replace the save's real world.

The mod also checks whether the base game has changed its standard world name. On the first
blueprint load in a world it warns if FactoryGame's map-rename table (`FindNewMapName`) renames
`Persistent_Level`, or if a base-game map (`/Game/FactoryGame/...`) loads under another name.
Redirects keep working either way; the warning tells you to update `StockWorldName` in
`Public/BlueprintWorldNames.h`.

This is intentionally a load-time fix:

- `.sbp` and `.sbpcfg` files are never edited;
- there is no prefix-based duplicate library (`PEP - ` / `RWG - `);
- nothing is added to FactoryGame's redirect table, so there is nothing to clean up between worlds;
- the same code runs in packaged clients and dedicated servers.

## Finish and build on Windows

The checked-in plugin targets the SML 3.12.0 starter-project snapshot (`GameVersion >=491125`).
When moving it to a Windows development machine:

1. Install the current SML starter project and its matching Unreal Engine toolchain.
2. Copy `Mods/BlueprintWorldBridge` into the starter project's `Mods` directory. If the
   starter project has moved conventional C++ mods under another directory, use its current
   Alpakit C++ template location.
3. Compare `BlueprintWorldBridge.uplugin`'s `GameVersion` and SML `SemVersion` with the starter
   project's `Mods/SML/SML.uplugin`; update this plugin descriptor if they differ.
4. Regenerate Visual Studio project files, then build `FactoryGame` as **Development Editor / Win64**.
5. Run `Tests/run-tests.ps1` from a Visual Studio Developer PowerShell.
6. Package and install with Alpakit, then test in a **Shipping** game build.

The Linux-side tests only compile the Unreal-independent world-name normalizer. A full plugin
compile and runtime test require the proprietary FactoryGame headers/binaries supplied by the
Windows starter project.

## Runtime verification

Enable logs. Loading a save should log:

```text
LogBlueprintWorldBridge: Current world is 'SkyblockLevel' (/PEP_Mod/...). Blueprints from other worlds will be redirected here; files are not modified.
```

Opening the blueprint library or placing a blueprint from another world should log, once per
source world:

```text
LogBlueprintWorldBridge: Redirecting blueprints from world 'Persistent_Level' to 'SkyblockLevel'.
```

Check these first, because they confirm assumptions that could not be tested without FactoryGame:

- **The hook fires.** If blueprints from another world fail and no "Redirecting" line appears,
  FactoryGame is not calling `FindNewBlueprintWorldMapName` on that path.
- **The current world is the save's world.** If the "Current world" line names a staging or
  blueprint-designer world after the save's own line, tighten the filter in
  `HandlePostWorldInitialization`.

FactoryGame loads a blueprint through `AFGBlueprintSubsystem::LoadStoredBlueprint` on three
separate paths, and each one needs to work:

- **designer**: loading it into a Blueprint Designer (loads into the game world);
- **preview**: the placement hologram (loads into FactoryGame's separate blueprint world);
- **construct**: building the placed hologram (loads into the game world).

The preview is the riskiest path. The hooked lookup is documented as resolving levels "in a
blueprint world", and the mod answers with the save's world name. If the preview shows the
wrong connections but construction is correct, this is where to look.

Then test blueprints containing wires, belts, and lifts on all three paths, in both directions:

1. normal -> PEP/Skyblock;
2. PEP/Skyblock -> normal;
3. RWG -> normal;
4. normal -> RWG;
5. a world the mod was never told about (for example Blank Map or HexFactory) -> normal;
6. multiplayer/dedicated server with the mod installed everywhere required by SML.

Confirm that connections survive placement and that saving or placing a blueprint does not
change its source files. A "standard world" warning on vanilla saves means a game update renamed
`Persistent_Level`; repeat the matrix after updating `StockWorldName`.
