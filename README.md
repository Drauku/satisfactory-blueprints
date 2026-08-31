# Satisfactory Blueprints

Version-controlled blueprint library for Satisfactory, shared directly with the
game through a symlink so that saving a blueprint in-game creates a file in this
repository.

## Why this exists

Steam Cloud syncs Satisfactory **saves** but not **blueprints**. Inspecting
`remotecache.vdf` for app 526870 shows 166 `.sav` entries and zero `.sbp`
entries, so a machine rebuild or prefix reset loses the entire blueprint library
while the save files come back automatically.

A git repository closes that gap and gives every machine that clones it a full
copy.

## Layout

Blueprints are grouped into one directory per game session, matching how
Satisfactory organises them:

```
1.2-forest/          Fields of Folly/     Forestry Department/
Starter Map/         Stoned for Success/  Trees and Riches/
chaotifactory/       platform-factory/    ~archive/
```

Each blueprint is a `.sbp` file paired with a `.sbpcfg` holding its metadata.
Both are required — a `.sbp` without its `.sbpcfg` does not appear in-game.

## How the game reaches these files

Satisfactory reads blueprints from inside its Proton prefix. That path is a
symlink to this repository:

```
/mnt/games/SteamLibrary/steamapps/compatdata/526870/pfx/drive_c/users/steamuser/
    AppData/Local/FactoryGame/Saved/SaveGames/blueprints
        -> /home/drauku/games/satisfactory/blueprints
```

Wine follows native symlinks, so the game reads and writes through it with no
special configuration. New blueprints appear in `git status` immediately.

The repository lives outside the prefix on purpose. Steam owns everything under
`compatdata/`, and a Proton version change or a "Delete Proton files" action
recreates that tree from scratch.

## Restoring the link

A prefix rebuild replaces the symlink with an ordinary directory. To repair it:

```bash
./restore-prefix-symlink.sh
```

The script is safe to run repeatedly. If it finds a real directory where the
symlink belongs, it copies any blueprints inside into this repository before
replacing it, and keeps the original alongside as
`blueprints.replaced-<timestamp>` rather than deleting it.

Close Satisfactory first. The game caches the blueprint list at load and
rewrites the directory on exit, which would discard anything added underneath a
running instance.

## Binary files

`.gitattributes` marks `.sbp` and `.sbpcfg` as binary. Without it, git may apply
line-ending conversion and corrupt them.

At roughly 66 MB across ~3,200 blueprints, with a median file around 9 KB, plain
git handles this comfortably. Blueprints are written once rather than edited
repeatedly, so history accumulates as new objects instead of deltas over
changing binaries, and Git LFS is not needed.
