# World Configuration

This folder is the runtime definition of every world the simulator can build.
There are two files with two jobs.

| file            | job                                                            |
|-----------------|----------------------------------------------------------------|
| `worlds.config` | wiring: which registry file to use, which world starts active  |
| `worlds.registry` | definitions: one `world=` block per world, geometry + textures |

Both are plain text.
`#` starts a comment on any line.

---

## world config parametrization

### `worlds.config`

Lines are `key=value` pairs (blank and `#` lines ignored, whitespace trimmed).

- `world=NAME` — the registered world that becomes active when the simulation starts.
- `registry=PATH` — a registry file that **replaces** the whole world table (REPLACE semantics: the file IS the full list; worlds from any earlier load are gone until loaded again).
- Paths are relative to the repository root.

Example:

```
# classic worlds
world=ConferenceWorld

# or a generated dataset (acts as if ReloadRegistry was called at startup)
registry=core_voxels/resources/generated/myds/myds.registry
world=myds_map000
```

Fallback rules (only applied when no `world=` matched):

- unknown `world=` name → the currently active world is kept.
- if no `registry=` key is present the classic `worlds.registry` is loaded as the table, unless the table is already non-empty (e.g. `ReloadRegistry` was called) so a runtime swap is not silently overwritten by the defaults.
- if the active world is no longer in the table after a registry swap, the first registered world becomes active so the build always has a concrete world.

### `worlds.registry`

Each world is a block started by `world=NAME`; every setting between two `world=` lines belongs to that block.
Settings are whitespace-separated `key=value` tokens, so several fit on one line.

```
world=ConferenceWorld
map=core_voxels/resources/map_images/map_paper.png
atlas=core_voxels/resources/textures/atlas.png
ground=core_voxels/resources/textures/grass.png
sky=core_voxels/resources/textures/stars.png
atlas_cols=2  atlas_rows=2
colors=red,green,blue,pink
tile.red=0  tile.green=1  tile.pink=2  tile.blue=3
object.red=tree  object.green=brick  object.pink=building  object.blue=bush
```

#### Required keys

A block missing any of these is skipped with a warning (`world_registry: skipping '<name>' ...`).

| key       | meaning                                             |
|-----------|-----------------------------------------------------|
| `map`     | map image path (the maze layout)                    |
| `atlas`   | texture atlas path (voxel surfaces)                 |
| `ground`  | ground texture path                                 |
| `sky`     | sky texture path                                    |

All paths are relative to the repository root (`core_voxels/resources/...`).

#### Optional keys

| key                  | meaning                                                        |
|----------------------|----------------------------------------------------------------|
| `atlas_cols=N`       | atlas grid columns (default 2)                                |
| `atlas_rows=N`       | atlas grid rows (default 2)                                   |
| `colors=a,b,...`     | palette this map image was drawn with (comma-separated names) |
| `tile.COLOR=idx`     | which atlas tile a color's voxels use                         |
| `object.COLOR=name`  | which object cluster a color becomes                          |
| `object_texture.NAME=idx` | explicit atlas tile for an object (overrides `tile.*`)   |

#### Supported names

Color names (`hue_by_name`): `white, red, orange, yellow, green, cyan, blue, pink, purple, brown`.

Object names (`object_by_name`): `tree, brick, bush, building, rock, sand, fence, puddle, pillar, crate`.

#### Textures

- If an object has an `object_texture`, that atlas tile wins.
- Otherwise the object falls back to `tile.COLOR` for the color it was mapped from.
- Objects that define neither keep the last-resort `textureFor()` lookup, preserving the legacy 2x2 classic behavior (`texture = -1`).

#### Map image rules

The map image is the maze.
Things that must hold for a world to play correctly:

- Both sides must be multiples of 16 (one chunk = 16x16 pixels).
- One pixel == one world unit; world coordinate == pixel coordinate.
- **Bright pixels (average RGB > 150) are empty / open ground** (`white`).
- **Colored pixels are solid voxels** — the color is matched by hue (45° tolerance) to the world's palette, then turned into the `object.*` cluster for that color.
- Dark pixels that match no palette color are treated as unbuilt but still collision blocking — avoid them unless you know what you are doing.
- No border around the map: the robot auto-spawns just outside the map corner and drives in, so a rim wall at the edge would trap it immediately.

#### Atlas grid

Tile `idx` sits at grid position `tile_z = idx / atlas_cols`, `tile_x = idx % atlas_cols` (matches `fetchTextureCoords`).
Worlds using the generated 16-tile shared atlas must set `atlas_cols=4  atlas_rows=4`.

---

## Runtime API (Python)

The pybind11 module loads `worlds.config` at import time, so `WORLDS` is populated before anything else happens.

```python
import voxel_sim as v

print(v.WORLDS)                                # tuple of registered world names

v.ReloadRegistry("path/to/dataset.registry")   # replace the whole table now
print(v.WORLDS)                                # refreshed after the reload

obs = v.Init()
obs = v.Reset("myds_map000", (32.0, 32.0, 90.0))   # world + (x, z, yaw_deg)
```

`ReloadRegistry(path)` clears the table and loads only that file, refreshing the `WORLDS` tuple; `Reset(world, position)` accepts any name currently in the table.
A persistent setup (surviving restarts) is done by setting `registry=` in `worlds.config` instead.

## Generate more worlds

See `scripts/worldgen/README.md` — the Python toolkit writes map images, the shared atlas and per-dataset registry files that this folder consumes.