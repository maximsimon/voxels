# World Generation Toolkit

Generates whole datasets of worlds for the Voxel World simulator: map images, a texture atlas
per set, a registry file, and a Python manifest describing every world and its sampled
parameters. Everything ships as the `scripts.worldgen` package.

---

## Quick start

```bash
# 1. generate a dataset (atlas sets + N maps + <name>.registry + <name>.worlds.py)
python3 -m scripts.worldgen.generate_all --dataset myds --count 24

# 2. check it in the real simulator: build every world, spawn alive, report lidar and timings
python3 scripts/worldgen/example_vecenv_rollout.py --dataset myds --timing
```

Point RL at it by setting `world_folder` in `scripts/rl/rl_config.py` to
`core_voxels/resources/generated/myds`. The folder is loaded directly, so no edit to
`core_voxels/resources/worlds/worlds.config` is needed.

Generated files land under `core_voxels/resources/generated/<dataset>/`; atlas sets are
written to `core_voxels/resources/textures/atlas_<set>.png`.

---

## Conventions every generated file must follow

- Map sides are multiples of 16 (one chunk = 16x16 pixels); world coordinate == pixel
  coordinate.
- Bright pixels (average RGB > 150) are empty/open ground; colored pixels are solid voxels.
- Emitted colors must sit inside the C++ matcher's 45° hue tolerance of a named hue center.
  `palette.py` enforces this; see "Color safety" below.
- Map images have **no border** — the robot auto-spawns at the origin corner and drives in.
- Atlas tiles lay out as a grid: tile `idx` is at `tile_z = idx / cols`,
  `tile_x = idx % cols`.
- The generated 4x4 atlas sets must be declared with `atlas_cols=4  atlas_rows=4`.

---

## Density tiers

Density is an absolute solid-pixel fraction, and it is **stratified** rather than sampled per
world: the trainer picks a random world every episode, so a dataset that happens to draw one
dense map would silently leave that regime untrained.

| tier     | band        | what it looks like                                     |
|----------|-------------|--------------------------------------------------------|
| `sparse` | 3% – 6%     | open ground, scattered single objects                   |
| `medium` | 12% – 18%   | maze walls or groves, still easy to traverse            |
| `dense`  | 20% – 32%   | tight lattice or thickets, real navigation problem      |

`generate_all.py` interleaves the tiers (`tier_plan`), so `count=24` gives 8 of each.

---

## Layouts

Layout and density are independent axes. Every layout fills to an **absolute** fraction, and
`generate_map` runs the chosen layouts in order, handing each the cumulative target
`target * (i + 1) / n`. Because a layout only ever adds solid pixels, the last one lands the
map in its density band whatever combination was used.

- `maze` — randomized-DFS lattice. Cell size, wall thickness and braid are all solved from
  the target: the lattice has a ceiling (`~1/cell` of the map), so the cell is the widest that
  still fits, thickness is raised only when nothing thinner reaches the band, and braid
  tunes the last percent or so.
- `columns` — pillars scattered on open ground. Spacing is derived from the budget, and a
  fraction cluster next to an existing column so sparse maps form groves.
- `rubble` — 1..3px blocks dropped in bursts, reading as thickets at low density and filling
  in at high density.

Combinations are layered, not blended: `maze+rubble` lays a maze, then scatters debris through
its corridors.

Two things run after the layouts, and both exist because of observed failures:

- **`_top_up`** — a layout that runs out of room (columns needs a clear ring per pillar) would
  leave a dense map in the medium tier. The shortfall is filled with the same small blocks.
- **`ensure_reachable`** — a maze can wall the spawn into a pocket that a clear disc does not
  prevent. Lanes are carved from the origin to the nearest unreachable open cell until enough
  open ground connects, and `validate.py` then measures the result.

---

## Color safety

The C++ (`core_voxels/src/map.cpp::getPixelHue`) does **not** use your color directly:

1. average RGB brightness > 150 → empty (no object)
2. otherwise the **nearest enabled hue center** within 45° wins

So a palette is only as good as its spacing. `orange` (30°) and `brown` (30°) share a center;
dark gray has hue 0 and reads as **red**; a color placed between two centers can be captured by
the wrong one. `palette.py` enumerates conflict-free subsets with `safe_subsets` /
`max_safe_palette` (8 colors fit the generated atlases), and `validate.py` re-checks each
palette against the C++ rule rather than trusting the sampler.

Atlas capacity limits palette size: the generated 4x4 sets have 16 slots, while the hand-made
2x2 atlases (`atlas.png`, `city_atlas.png`) only support 4 colors.

---

## Module by module

### `mapgen.py` — the pixel canvas

- `MapCanvas(width, height)` — grid of pixels with parallel `pixels`, `empty` and `protected`
  arrays. `paint` / `rect` / `line` / `paint_mask` write; `empty` is derived from brightness via
  `palette.is_white` unless overridden with `empty=`.
- `reserve_spawn(radius)` / `clear_protected()` — a disc around the origin kept clear, so no
  layout can wall the spawn in.
- `reachable_from_origin(mc)` — numpy dilation flood returning the reachable mask and fraction
  of open ground. Whole-array ops rather than a per-cell queue, so it is cheap enough to check
  on every generated world.
- `save_png` / `from_png` — I/O.

### `layouts.py` — the generators

- `generate_map(width, height, layouts, target_fill, styles, seed=..., spawn_radius=...)` —
  run the layouts, clear the spawn disc, top up, ensure reachability; returns the canvas and a
  per-layout report for the manifest.
- `plan_layouts(rng, allowed, max_layouts=3)` — pick 1..3 distinct layouts for one world.
- `DENSITY_TIERS`, `tier_bounds`, `SPAWN_CLEAR_RADIUS`, `MIN_REACHABLE`.

### `palette.py` — colors and objects

- `COLOR_RGB`, `HUE_CENTER`, `HUE_TOL_DEG` — mirror of the C++ hue rules.
- `is_white(rgb)`, `resolves_to(rgb, names)`, `conflicts(names)`, `safe_subsets(names, n)`,
  `max_safe_palette(names, cap)`.
- `OBJECT_LIBRARY` — object name → cluster size, voxel count and the layout roles that use it,
  mirroring `core_voxels/resources/worlds/objects.hpp`. `objects_for_role(role, max_voxels)`
  is how a tier trades appearance against rebuild cost.

### `atlas.py` — texture atlases

- `ATLAS_SETS` — named 16-slot grids (`park`, `stone`, `wild`) over the source photos.
- `OBJECT_SOURCES` / `object_tiles(set_name)` — a suitable tile per object, distinct where the
  set allows. The C++ prefers an object's own tile over its color's (`map.cpp::genObject`), so
  this is what makes a tree look like a tree instead of like whatever the color's slot held.
- `build_atlas`, `build_atlas_sets`, `atlas_path`, `atlas_sets`, `tile_uv`, `slice_atlas`.

### `pathgen.py` / `obstacles.py` — corridor and obstacle helpers

- `draw_path(canvas, waypoints, width, color)` — carves a corridor along pixel waypoints.
- `place_around_path`, `place_box` — obstacles that stay off a corridor.

### `registry.py` — writing registry files

- `world_block(...)` — one `world=` block as text.
- `write_registry(path, blocks)` — the whole registry file.
- Keys map 1:1 to `core_voxels/resources/worlds/README.md`.

### `dataset.py` — sampling a whole dataset

- `generate_dataset(name, count, ...)` — samples every appearance axis independently per world
  (layout combination, density tier, colors, objects, atlas set, ground, sky), generates and
  validates each map, and writes the registry plus the manifest.
- `tier_plan` — the stratified tier order.
- `write_manifest` — `<name>.worlds.py`, importable Python holding `DATASET` and `WORLDS`.
  Importable rather than JSON because the registry already keys everything by name and a real
  Python module can be read and diffed without a parser.

### `validate.py` — refusing to ship a world the C++ cannot build as described

- `validate_map` — dimensions, fill inside the tier band, spawn disc clear, origin open,
  reachable open fraction, colors resolvable / non-conflicting / actually used.
- `validate_world` — objects exist in `OBJECT_LIBRARY`, tile indices inside the atlas, no two
  colors sharing a tile, no `object_texture` on an object several colors map to, texture paths
  exist.
- `validate_dataset` — both halves per world, plus duplicate names.
- `report(failures)` — prints and returns the count that failed.

### `generate_all.py` — CLI

| flag                    | default             | meaning                                    |
|-------------------------|---------------------|--------------------------------------------|
| `--dataset`             | (required)          | folder + registry + manifest name          |
| `--count`               | 12                  | number of worlds                           |
| `--width`/`--height`    | 256                 | map size in pixels (multiple of 16)        |
| `--seed`                | 0                   | world *i* uses `seed + i`                  |
| `--layouts`             | all                 | comma-separated layouts to draw from       |
| `--layouts-per-world`   | 3                   | max layouts layered per world              |
| `--density`             | all tiers           | comma-separated density tiers              |
| `--palette-min`/`-max`  | 4 / 7               | colors per world (conflict-free subsets)   |
| `--atlas-cols`/`-rows`  | 4 / 4               | atlas grid                                 |
| `--atlas-set`           | random per world    | force one named set                        |
| `--ground-pool`         | grass, desert, asphalt | ground textures                          |
| `--sky-pool`            | stars, sky_clouds   | sky textures                               |
| `--spawn-radius`        | 10                  | clear disc around the origin               |
| `--no-validate`         | off                 | skip validation (not recommended)          |
| `--atlas-only`          | off                 | build atlas sets only                      |
| `--list-layouts` / `--list-atlases` | —          | print the known names and exit             |

### `example_vecenv_rollout.py` — simulator check

Generation proves a map is self-consistent; this proves the C++ agrees. For each world it
resets in, checks the robot spawned alive, steps it, requires non-zero lidar, records a camera
signature, and (with `--timing`) reports the destroy+rebuild cost every episode end pays. It
also reports pairwise camera differences, which is the cheapest proof that two worlds really
look different.

```bash
python3 scripts/worldgen/example_vecenv_rollout.py --dataset myds --timing
python3 scripts/worldgen/example_vecenv_rollout.py --dataset myds --mosaic
```

Note it never initializes the simulation in the parent process — `VecEnv`'s workers fork and
run their own `Configure`/`Init`, and the parent already having a window leaves the inherited
state unable to set `show_window`.

---

## Extending

- **New texture**: drop the file in `core_voxels/resources/textures/`, add it to an
  `ATLAS_SETS` entry (or a new set) and to `OBJECT_SOURCES` if an object should use it.
- **New layout**: write a `@layout("name")` function taking `(mc, rng, until_fill, style)` and
  returning a stats dict. Fill to the absolute `until_fill`; do not worry about the spawn disc
  or connectivity, `generate_map` handles both afterwards.
- **New density tier**: add a band to `DENSITY_TIERS`.
- **Extending a dataset**: `append_registry` adds worlds to an existing registry. New
  *datasets* should get their own name — `generate_dataset` rewrites the registry it owns.