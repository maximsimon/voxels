# World Generation Toolkit

Python scripts that generate entire worlds for the Voxel World simulator: a shared 16-tile texture atlas, map image variants, and per-dataset registry files.
Everything ships as the `scripts.worldgen` package. Run `generate_all.py` to generate a dataset end-to-end.

---

## Quick start

Generate a dataset (atlas + N map variants + `<name>.registry`) and point the simulator at it:

```bash
# 1. generate
python -m scripts.worldgen.generate_all --dataset myds --count 50 --width 256 --height 256

# 2. point the simulator at the dataset (edit core_voxels/resources/worlds/worlds.config)
registry=core_voxels/resources/generated/myds/myds.registry
world=myds_map000

# 3. run a visualisation test/sanity check (omit --show for headless run)
PYTHONPATH=scripts/bare_bones:build python scripts/worldgen/example_vecenv_rollout.py --show
```

The generated files land under `core_voxels/resources/generated/<dataset>/`; the shared atlas is written to `core_voxels/resources/textures/shared_atlas.png` (overwritten on each run).

---

## Conventions every generated file must follow

- Map sides are multiples of 16 (one chunk = 16x16 pixels); world coordinate == pixel coordinate.
- Bright pixels (average RGB > 150) are empty/open ground; colored pixels are solid voxels.
- Map images have **no border** — the robot auto-spawns just outside the map corner and drives in.
- Atlas tiles lay out as a grid: tile `idx` is at `tile_z = idx / cols`, `tile_x = idx % cols` (matches `fetchTextureCoords`).
- Any registry block that uses the generated 4x4 shared atlas must set `atlas_cols=4  atlas_rows=4`.

---

## Module by module

### `atlas.py` — shared texture atlas

- `ATLAS_TILES`: hand-maintained manifest `index -> (name, image_path)`. This is the place to add real textures later; indices repeat the last tile to fill the grid when there are fewer unique images than cells.
- `build_atlas(cols=4, rows=4, out=None)` — square-crops each source to its center, resizes it to 256px, and lays all tiles into the `cols` x `rows` grid. Returns `{index: tile_name}` for registry writers. Default `out` is `core_voxels/resources/textures/shared_atlas.png`.
- `tile_uv(index, cols=4, rows=4)` — returns `(u_min, v_min, u_max, v_max)` for a tile index (inspection).
- `slice_atlas(path, cols=4, rows=4)` — splits an atlas PNG back into individual tile images (debugging).

### `palette.py` — RGB palette

- `COLOR_RGB` — dict of color name -> `(r, g, b)` for the palette the C++ hue matcher accepts.
- `parse_colors(["red", "green"])` — resolves a list of names to `(name, r, g, b)` tuples; skips unknown names with a warning.
- Colors are chosen so each pixel's hue sits inside the C++ 45° hue tolerance.

### `mapgen.py` — map images

- `MapCanvas(width, height)` — an all-white (fully open) pixel grid. `paint(x, z, color)`, `rect(x0, z0, x1, z1, color)` and `line(...)` write colored pixels; emptiness is derived from brightness. `free_cells()` returns open pixels, `white_fraction()` the open ratio, `save_png(path)` / `from_png(path)` do I/O.
- `generate_variant(width, height, rooms=4, colors=None, obstacles=12, seed=None)` — a map_paper-style variant with white open ground, `rooms` hollow colored rectangles (a random door gap on every side) and `obstacles` scattered colored blocks. Default output is 89-92% open, matching the classic maps' look.

### `pathgen.py` — walkable corridors

- `draw_path(canvas, waypoints, width=1, color)` — carves a corridor along a list of pixel waypoints as empty cells. Returns the set of center-line cells, which `obstacles.py` uses to keep obstacles off the corridor.

### `obstacles.py` — placing obstacles

- `place_around_path(canvas, path_cells, color, count=20, min_dist=3, rng=None)` — drops colored blocks near but not on the path.
- `place_box(canvas, cx, cz, bw, bh, color)` — a rectangular obstacle centered at `(cx, cz)`; returns the occupied cells.

### `registry.py` — writing registry files

- `world_block(name, *, map_path, atlas_path, ground_path, sky_path, atlas_cols=4, atlas_rows=4, colors, tiles, objects, object_textures)` — builds one `world=` block as text.
- `write_registry(path, blocks)` — writes a registry file from a list of block strings.
- `append_registry(path, block)` — appends one block to a file.
- Keys map 1:1 to the format documented in `core_voxels/resources/worlds/README.md`.

### `dataset.py` — full dataset generation

- `generate_dataset(name, count=10, width=256, height=256, rooms=4, base_seed=0, atlas_cols=4, atlas_rows=4, ground="grass.png", sky="stars.png")` builds the shared atlas, generates `count` map variants, and writes one `<name>.registry`. Returns the path to the registry file.
- Maps and registry land under `core_voxels/resources/generated/<name>/`; paths inside the registry use the repo-root relative convention the simulator expects.

### `generate_all.py` — CLI entry point

```
python -m scripts.worldgen.generate_all --dataset myds --count 50 --width 256 --height 256 --rooms 4 --seed 0
```

| flag          | default    | meaning                                    |
|---------------|------------|--------------------------------------------|
| `--dataset`   | (required) | dataset name (folder + registry name)      |
| `--count`     | 10         | number of map variants                     |
| `--width`     | 256        | map width in pixels (multiple of 16)       |
| `--height`    | 256        | map height in pixels (multiple of 16)      |
| `--rooms`     | 4          | walled rooms per map                       |
| `--seed`      | 0          | base RNG seed                              |
| `--atlas-cols`| 4          | shared atlas columns                       |
| `--atlas-rows`| 4          | shared atlas rows                          |
| `--ground`    | grass.png  | ground texture filename                    |
| `--sky`       | stars.png  | sky texture filename                       |
| `--atlas-only`| off        | build the atlas, skip map generation       |

### `example_vecenv_rollout.py` — headless demo

Resets into every registered world, steps a few times, prints the travelled positions.

```
PYTHONPATH=scripts/bare_bones:build python scripts/worldgen/example_vecenv_rollout.py [spawn_x] [spawn_z]
```

`spawn_x`/`spawn_z` default to `(32, 32)` and should be an open map cell at least a few pixels from any wall.

---

## Extending

- Add a real texture: drop the file under `core_voxels/resources/textures/` and point its index in `ATLAS_TILES` at it.
- Change wall shapes: pass your own `colors` to `generate_variant`, or layer `MapCanvas` primitives for bespoke layouts.
- One dataset per `dataset()` call; append more worlds to a registry with `append_registry`.
- Any registry file this package writes is consumed by the simulator exactly like the classic `worlds.registry` — see `core_voxels/resources/worlds/README.md`.
