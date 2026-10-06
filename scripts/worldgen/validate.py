# validate.py - refuse to ship a world the C++ cannot build the way it was described.
#
# every rule here exists because the alternative is a silently wrong world: a map the hue
# matcher reads as a different color than intended, an atlas too small for the palette, a
# spawn sealed inside a wall, or - the failure that produced the near-empty test420 maps -
# a fill fraction far below the tier it was supposed to be in.

from __future__ import annotations

from pathlib import Path

import numpy as np

from .layouts import SPAWN_CLEAR_RADIUS, tier_bounds
from .mapgen import CHUNK_WIDTH, MapCanvas, reachable_from_origin
from .palette import (COLOR_RGB, OBJECT_LIBRARY, is_ambiguous, is_white, resolves_to)

# the reserved spawn disc costs a little fill on a sparse map; allow the band to slip by this
FILL_TOLERANCE = 0.015

# open ground should stay reachable - a map whose free cells are mostly sealed off is a map
# the robot cannot explore, even when the numbers look right
MIN_REACHABLE = 0.55

# a pixel with this little spread between channels reads as grey, and grey has hue 0
MIN_CHANNEL_SPREAD = 40


def _spawn_disc_clear(mc: MapCanvas, radius: int) -> bool:
	ys, xs = np.ogrid[:mc.height, :mc.width]
	disc = ((xs - 0.5) ** 2 + (ys - 0.5) ** 2) <= radius * radius
	return bool(mc.empty[disc].all())


def _unique_colors(mc: MapCanvas) -> list[tuple[int, int, int]]:
	return [tuple(int(v) for v in c) for c in np.unique(mc.pixels.reshape(-1, 3), axis=0)]


def validate_map(mc: MapCanvas, *, tier: str, target_fill: float, colors: list[str],
                 layouts: list[str]) -> list[str]:
	"""Structural checks on the pixel data. Returns a list of problems (empty = good)."""
	errors: list[str] = []

	if mc.width % CHUNK_WIDTH or mc.height % CHUNK_WIDTH:
		errors.append(f"map {mc.width}x{mc.height} is not a multiple of {CHUNK_WIDTH}")

	lo, hi = tier_bounds(tier)
	fill = mc.fill_fraction()
	if not (lo - FILL_TOLERANCE <= fill <= hi + FILL_TOLERANCE):
		errors.append(f"fill {fill:.2%} outside tier '{tier}' ({lo:.0%}-{hi:.0%}, "
					  f"target {target_fill:.2%})")

	if not _spawn_disc_clear(mc, SPAWN_CLEAR_RADIUS):
		errors.append(f"spawn disc of radius {SPAWN_CLEAR_RADIUS} around the origin is blocked")
	if not mc.is_empty(0, 0):
		errors.append("origin cell (0,0) is solid - the robot would spawn inside a wall")

	_, reachable = reachable_from_origin(mc)
	if reachable < MIN_REACHABLE:
		errors.append(f"only {reachable:.0%} of open ground is reachable from the origin "
					  f"(need {MIN_REACHABLE:.0%})")

	present = {c for c in _unique_colors(mc) if not is_white(*c)}
	unknown = [c for c in present if c not in set(COLOR_RGB.values())]
	if unknown:
		errors.append(f"{len(unknown)} map color(s) are not in the palette, e.g. {unknown[0]}")

	grey = [c for c in present if (max(c) - min(c)) < MIN_CHANNEL_SPREAD]
	if grey:
		errors.append(f"{len(grey)} near-grey color(s) present, e.g. {grey[0]} - grey has hue 0 "
					  f"and builds as red")

	declared = set(COLOR_RGB[c] for c in colors if c in COLOR_RGB)
	undeclared = present - declared
	if undeclared:
		errors.append(f"{len(undeclared)} drawn color(s) missing from the world's palette, "
					  f"e.g. {next(iter(undeclared))}")

	for name in colors:
		actual = resolves_to(name, colors)
		if actual != name:
			errors.append(f"color '{name}' builds as '{actual}' when the world enables "
						  f"{colors} - pick a conflict-free palette")
		elif is_ambiguous(name, colors):
			errors.append(f"color '{name}' ties between two enabled hue centers")

	unused = [n for n in colors if COLOR_RGB.get(n) not in present]
	if unused:
		errors.append(f"declared but never drawn: {', '.join(unused)}")

	for name in layouts:
		if name not in ("maze", "columns", "rubble"):
			errors.append(f"unknown layout '{name}'")

	return errors


def validate_world(meta: dict, mc: MapCanvas, repo_root: Path) -> list[str]:
	"""Everything validate_map cannot see: objects, tiles and texture paths."""
	errors: list[str] = []

	for color, obj in (meta.get("objects") or {}).items():
		if obj not in OBJECT_LIBRARY:
			errors.append(f"color '{color}' maps to unknown object '{obj}' - the C++ "
						  f"object_by_name() would return null and draw nothing")

	capacity = int(meta["atlas_cols"]) * int(meta["atlas_rows"])
	for holder in (meta.get("tiles") or {}, meta.get("object_textures") or {}):
		for key, idx in holder.items():
			if not 0 <= int(idx) < capacity:
				errors.append(f"tile index {idx} for '{key}' is outside the atlas "
							  f"({meta['atlas_cols']}x{meta['atlas_rows']} = {capacity} slots)")

	if len(set((meta.get("tiles") or {}).values())) != len(meta.get("tiles") or {}):
		errors.append("two colors share one atlas tile - they will render identically")

	obj_tiles = meta.get("object_textures") or {}
	if len(set(obj_tiles.values())) != len(obj_tiles):
		errors.append("two objects share one atlas tile - they will render identically")

	# an object texture is not cosmetic: genObject prefers it over the color's tile, so two
	# colors that share an object name would render as one photo and a color->tile entry for
	# that color would never be read
	shared = [obj for obj in obj_tiles
	          if sum(1 for o in (meta.get("objects") or {}).values() if o == obj) > 1]
	for obj in shared:
		errors.append(f"object '{obj}' is mapped from more than one color but has an "
					  f"object_texture - the per-color tiles for it are dead entries")

	for obj in (meta.get("object_textures") or {}):
		if obj not in OBJECT_LIBRARY:
			errors.append(f"object_texture set for unknown object '{obj}'")

	for key in ("map", "atlas", "ground", "sky"):
		rel = meta.get(key)
		if not rel:
			errors.append(f"world is missing '{key}'")
			continue
		if not (repo_root / rel).exists():
			errors.append(f"{key} file does not exist: {rel}")

	return errors


def validate_dataset(metas: list[dict], repo_root: Path) -> dict[str, list[str]]:
	"""Validate every world, plus the dataset-level checks no single world can make.

	Both halves run: validate_map covers the pixels (tier, spawn, colour, reachability) and
	validate_world covers what only the registry knows (objects, tiles, texture paths). Called
	standalone on maps that were loaded rather than just generated, checking only the registry
	would pass a dataset whose PNGs do not match the tiers they claim.
	"""
	failures = {}
	for m in metas:
		mc = MapCanvas.from_png(repo_root / m["map"])
		failures[m["name"]] = validate_map(m, mc) + validate_world(m, mc, repo_root)

	names = [m["name"] for m in metas]
	for name in sorted({n for n in names if names.count(n) > 1}):
		failures.setdefault(name, []).append("duplicate world name in the registry")

	if not metas:
		failures["<dataset>"] = ["dataset has no worlds"]
	return failures


def report(failures: dict[str, list[str]]) -> int:
	"""Print a validation report; returns the number of worlds that failed."""
	bad = 0
	for name, errors in sorted(failures.items()):
		if not errors:
			continue
		bad += 1
		print(f"validate: FAIL {name}")
		for e in errors:
			print(f"    - {e}")
	if bad == 0:
		print(f"validate: {len(failures)} world(s) OK")
	return bad