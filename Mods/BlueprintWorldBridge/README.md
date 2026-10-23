# Blueprint World Bridge

An experimental Satisfactory Mod Loader (SML) C++ mod that lets one factory blueprint be
placed in saves whose mods use different persistent world names.

The known names are:

| World family | Blueprint world name |
|---|---|
| Vanilla and SatisfactoryPlus | `Persistent_Level` |
| Platform Expansion Program and Skyblock | `SkyblockLevel` |
| Random World Generator | `ProceduralWorldLevel` |

## Approach

FactoryGame already exposes a blueprint-specific redirect API:
`UFGSaveSystem::AddBlueprintMapRedirector`. When `AFGBlueprintSubsystem` begins play, the
mod identifies the active world's map name and redirects the other known names to it.

This is intentionally a load-time fix:

- `.sbp` and `.sbpcfg` files are never edited;
- there is no prefix-based duplicate library (`PEP - ` / `RWG - `);
- redirect state is refreshed for each world, and only redirects successfully installed by
  this mod are removed during world changes or module shutdown;
- a redirect already registered by FactoryGame or another mod is left unchanged and logged;
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

Enable logs and look for a line like:

```text
LogBlueprintWorldBridge: Installed 2 blueprint world redirect(s) for 'SkyblockLevel'. Blueprint files remain unchanged.
```

Then test blueprints containing wires, belts, and lifts in both directions:

1. normal -> PEP/Skyblock;
2. PEP/Skyblock -> normal;
3. RWG -> normal;
4. normal -> RWG;
5. multiplayer/dedicated server with the mod installed everywhere required by SML.

Confirm that connections survive placement and that saving or placing a blueprint does not
change its source files. If the module compiles but no log line appears, verify the
`AFGBlueprintSubsystem::BeginPlay` hook against the current starter-project headers.

## Adding another world family

Add its actual persistent level name to `KnownBlueprintWorldNames` in
`Public/BlueprintWorldNames.h`, add a normalizer test, and repeat the cross-world runtime matrix.
