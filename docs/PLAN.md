# Plan: unified blueprint CLI, then an in-game load fix

## Goal

Replace the three separate tools in `scripts/` with one command-line tool (`sfbp`), then
move the core fix into a Satisfactory mod so blueprints work across map types
without conversion.

## Current state

| Tool | Language | Does |
|---|---|---|
| `scripts/link-blueprints.sh` | bash | Symlinks each game session's blueprint folder to a repo folder, per `links.conf` |
| `scripts/dedupe-blueprints.sh` | bash | Collapses per-session copies into `_master`, keeps divergent copies in `_variants` |
| `scripts/bp-convert/` | Node | Rewrites the level name inside blueprints and swaps the `PEP - ` / `RWG - ` prefix |

### The underlying problem

The game stamps every object and connection in a blueprint with the level name
of the map it was saved in, and does not translate it on load:

| Map type | Level name | Prefix |
|---|---|---|
| Vanilla, SatisfactoryPlus | `Persistent_Level` | none |
| Platform Expansion Program, Skyblock | `SkyblockLevel` | `PEP - ` |
| Random World Generator | `ProceduralWorldLevel` | `RWG - ` |

A blueprint placed in a map with a different level name loses every power wire
and some belt and lift connections (ends that sit exactly on a port can still
snap back). Confirmed in both directions for normal and PEP maps, and from RWG
into a normal map.

`links.conf` maps sessions of every map type to the same `_master`, so a single
shared library cannot work for all of them.

## Phase 1: `sfbp`, a single CLI

One Node tool in `scripts/sfbp/`, absorbing `bp-convert` and replacing the bash
scripts (or keeping them as thin wrappers during the transition).

| Command | Purpose |
|---|---|
| `sfbp status` | Sessions, their detected map type, the folder each links to, and mismatches |
| `sfbp link` | `link-blueprints.sh --apply`, choosing the library for each session's map type |
| `sfbp dedupe` | `dedupe-blueprints.sh` |
| `sfbp convert` | `bp-convert` |
| `sfbp sync` | Copies blueprints saved in one map type's library into the others, converted |
| `sfbp doctor` | Missing `.sbpcfg`, unreadable files, mixed level names, dangling references |

### Design

1. **Detect map type from the save.** Saves contain the level name
   (`platform-factory`: `SkyblockLevel`; SFP and `chaotifactory`:
   `Persistent_Level`), so sessions can be classified without asking.
2. **One library per map type.** `_master` (normal), `_master-pep`,
   `_master-rwg`. `link` points each session at the library for its type;
   `sync` converts new or changed blueprints across libraries, tracking which
   copy is newest so edits are not overwritten.
3. **Config.** Keep `links.conf` working; add an optional map-type field to
   override detection.
4. **Portability.** Find the Steam library and Proton prefix (or the Windows
   save path) instead of hard-coding `/mnt/games/...`, so the tool can be shared.
5. **Safety.** Carry over from `bp-convert`: list everything, confirm before
   writing, `--dry-run`, originals kept in `archive/`, case-insensitive name
   clash checks, re-parse every output before writing.
6. **Game-running check.** `link-blueprints.sh` uses `pgrep -f`, which also
   matches any shell whose command line contains the process name. Match the
   process name exactly instead.
7. **Tests.** Small fixture blueprints of each map type; round-trip conversion
   tests; link and sync tests against a temporary fake prefix.

### Steps

1. Scaffold `scripts/sfbp/` and move the `bp-convert` core into a module.
2. `status` with save-based map-type detection.
3. `link` and `dedupe` ported from bash, with the archive and exclusion rules
   they have today.
4. Per-type libraries and `sync`.
5. `doctor`.
6. Docs, then retire or wrap the bash scripts.

## Phase 2: the addon

- **Load-time fix (scaffolded).** `Mods/BlueprintWorldBridge` uses FactoryGame's
  blueprint-specific `UFGSaveSystem::AddBlueprintMapRedirector` API when
  `AFGBlueprintSubsystem` begins play. This is narrower than rewriting stored data in a
  `LoadStoredBlueprint` hook: it leaves blueprint files untouched and asks the game's own
  deserializer to resolve known source levels to the active world.
- **Cost.** C++ against SML's custom Unreal Engine with the Windows toolchain;
  building from Linux is awkward.
- **Remaining validation.** Build against the current Windows starter project, package with
  Alpakit, and run the cross-world plus multiplayer matrix in the mod README. If the current
  game no longer honors blueprint map redirectors for object references, fall back to a
  `LoadStoredBlueprint`-adjacent deserialization hook.
- **Alternative.** The PEP mod author (`maker_dude`) may add the same fix to PEP,
  which would cover the PEP case without a separate mod.

### Not part of this plan: an in-game blueprint manager

An in-game library manager (browse, categorise, sync) is out of scope at this time.
Satiscomfory's BlueprintManagerPlus already covers that ground, although it currently
crashes after loading a save (an issue for it is drafted elsewhere).
A manager of our own is only a possible much-later addition if more addons follow;
the addon in this plan is the "world name" conversion fix alone.

## Open decisions

1. Node for the whole tool, with the bash scripts retired or kept as wrappers?
2. Per-type libraries (`_master`, `_master-pep`, `_master-rwg`), or one normal
   library converted on demand when a session needs it?
3. Pursue the load-time fix as our own mod, or ask `maker_dude` to add it to PEP first?
