# seedgen

Generates a randomizer spoiler JSON from a preset, using the game's own generation code. It needs no window, ROM, or o2r files.

## Build

The target isn't part of the default build, and it requires Linux on x86-64.

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --target seedgen
```

The binary is `build/soh/seedgen/seedgen`. It's fully static if the toolchain has a static libc, such as on Debian or Ubuntu.

## Use

```sh
seedgen --preset PRESET.json --seed SEED --out OUT.json
```

| Exit code | Meaning |
|---|---|
| 0 | `OUT.json` was written. |
| 1 | The fill failed for this seed. Try another seed. |
| 2 | Bad arguments, an unreadable preset, or a setup error. Don't retry. |

- `--mq` generates as if Master Quest files are present. Without it, Master Quest settings are forced off, the same as in a game with no Master Quest ROM.
- `--verbose` prints the generation log.
- Run one process per seed, as generation uses global state.
- Use a build of the same commit as your players' game. Generation changes between commits, and the game doesn't reject a spoiler from a different build.

## Maintenance

Three things can need updating when the randomizer changes:

- **New undefined symbol at link time:** read what calls it first. If generation needs it, add the real source to `CMakeLists.txt`. If it's gameplay or drawing code, add it to `game_traps.inc`.
- **`location table has no entry for check`:** a new location file needs its source in `CMakeLists.txt` and its registrar called in `InitRandomizer`. If the game really has no location for that check, add it to `sNoLocation`.
- **`game-only function was called`:** generation now reaches gameplay code, and that function needs a real implementation.

## Testing

To check generation against the game, replay a spoiler log made by a build of the same commit. Only the version, branch, and commit lines should differ:

```sh
seedgen --from-spoiler GAME_SPOILER.json --out replay.json
diff GAME_SPOILER.json replay.json
```
