# Satisfactory Blueprints

Version-controlled blueprint library, linked into Satisfactory per session so
that saving a blueprint in-game writes a file in this repository.

## Why this exists

Steam Cloud syncs Satisfactory **saves** but not **blueprints**. Inspecting
`remotecache.vdf` for app 526870 shows 166 `.sav` entries and zero `.sbp`
entries, so a machine rebuild or prefix reset loses the blueprint library while
save files come back automatically. A git repository closes that gap.

## How the game finds blueprints

Satisfactory reads from `SaveGames/blueprints/<session name>/`, where the
session name comes from the save itself — `Sandifactory_autosave_1.sav` belongs
to session `Sandifactory`. Blueprints are therefore scoped to a session, not
shared globally.

Each mapped session gets its own symlink into this repository:

```
SaveGames/blueprints/Trees and Riches -> <repo>/Trees and Riches
SaveGames/blueprints/Starter Map      -> <repo>/Starter Map
```

Linking individual folders rather than the whole `blueprints/` directory keeps
each session's library separate, and lets several sessions share one folder.

## Layout

```
_master/     every blueprint, one copy each (637)
_variants/   older versions of blueprints whose name has several distinct contents (46)
```

Satisfactory scopes blueprints per session, so a library used in several worlds
was previously stored once per world — 3,230 files holding 668 unique
blueprints. `_master` holds one copy of each and every session links to it, so a
blueprint saved in any world is immediately available in all of them.

Where one name carried several distinct contents, the newest is the master copy
and the rest are kept under `_variants/<name>/<source-folder>-<hash>.sbp`.
Nothing is discarded; `_variants` is simply not linked into any session.

Two blueprints have no `.sbpcfg` partner anywhere — `acrifib - 600MW Power Plant
Mk.1` and `coal generator x36`. They will not appear in-game. This predates the
deduplication; no config for either exists in any recovered collection.

## Rebuilding the deduplication

`dedupe-blueprints.sh` collapses per-session folders into `_master` and
`_variants`. It is only needed if per-session folders reappear, such as after
importing another machine's library.

## Configuring the links

`links.conf` maps sessions to folders:

```
<session name> = <repo folder>
```

Several sessions may point at the same folder. Because Satisfactory copies
blueprints per session, a library duplicated across five worlds occupies five
copies on disk; mapping those sessions to one shared folder stores it once and
keeps every world in sync.

To see what is available:

```bash
./link-blueprints.sh --list
```

That prints session names discovered in your saves, the folders in this
repository, and the links currently in the prefix. Session discovery is
best-effort — older saves carry a `sessionName=` token, newer ones do not — so
treat it as a hint and confirm names against your save files.

To apply the mapping:

```bash
./link-blueprints.sh --apply
```

Safe to run repeatedly. Where a real directory sits in place of a link, its
blueprints are copied into the mapped repo folder first, and the directory is
kept alongside as `<name>.replaced-<timestamp>` rather than deleted.

Close Satisfactory before applying. The game caches the blueprint list at load
and rewrites the directory on exit, discarding anything added underneath a
running instance.

## Recovering after a Proton update

Steam owns everything under `compatdata/`. A Proton version change or a "Delete
Proton files" action recreates that tree and removes the symlinks. The
repository is outside the prefix for this reason, so recovery is:

```bash
./link-blueprints.sh --apply
```

## Binary files

`.gitattributes` marks `.sbp` and `.sbpcfg` as binary; without it git may apply
line-ending conversion and corrupt them. Each blueprint is a `.sbp` paired with
a `.sbpcfg` holding its metadata — a `.sbp` without its partner does not appear
in-game.

At roughly 66 MB with a median file near 9 KB, plain git handles this
comfortably. Blueprints are written once rather than edited repeatedly, so
history accumulates as new objects rather than deltas over changing binaries,
and Git LFS is unnecessary.
