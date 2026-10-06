# palette.py - the colors a generated map may draw with, mirroring the C++ hue pipeline.
#
# map.cpp::getPixelHue classifies a pixel in two steps: a pixel whose average RGB exceeds
# white_gray_threshold (150) is an empty cell, and anything else is handed to the hue
# centers the world enabled - the NEAREST center wins outright, hue is the only channel
# that counts (there is no saturation test), and a pixel further than hue_tolerance_deg
# (45) from every enabled center becomes "unknown", which buildVoxelWorldMesh drops.
#
# two consequences the generator has to respect:
#   - a near-grey pixel has hue 0, which is red's center, so greys silently build as red.
#     never emit them; see is_white() / the _COLOR_TABLE values, all of which sit well
#     under the brightness threshold.
#   - colors are NOT independent. Enabling one steals the hues of any other whose pixels
#     sit closer to the newly enabled center, so a world may only draw with a subset in
#     which every color still resolves to itself. safe_subsets() enumerates those, and
#     requires an UNAMBIGUOUS winner so the result does not depend on the order the C++
#     happens to iterate its enabled centers in.

import colorsys
from functools import lru_cache
from itertools import combinations

# C++ color_hue_mapping.hpp centers, in the order getPixelHue compares them
HUE_ORDER = ["red", "orange", "yellow", "green", "cyan", "blue", "purple", "pink", "brown"]
HUE_CENTER = {
	"red": 0.0,
	"orange": 30.0,
	"yellow": 60.0,
	"green": 120.0,
	"cyan": 180.0,
	"blue": 240.0,
	"purple": 270.0,
	"pink": 320.0,
	"brown": 30.0,
}
HUE_TOL_DEG = 45.0
WHITE_GRAY_THRESHOLD = 150

# (name, r, g, b) - every hue verified to sit inside 45 deg of its own center, and every
# average RGB well under 150 so none of them is mistaken for open ground. "black" is gone
# on purpose: there is no black HUE_TYPE, and its grey hue 0 resolves to red.
_COLOR_TABLE = [
	("red",    180,   0,   0),
	("orange", 200, 128,   0),
	("yellow", 180, 180,   0),
	("green",    0, 180,   0),
	("cyan",     0, 180, 180),
	("blue",     0,   0, 180),
	("purple", 128,   0, 180),
	("pink",   200, 100, 140),
	("brown",  140,  80,  20),
]

COLOR_RGB = {name: (r, g, b) for name, r, g, b in _COLOR_TABLE}

# display names for registry files (the C++ hue_by_name accepts lowercase names)
HUE_NAMES = [name for name, _, _, _ in _COLOR_TABLE]

# every color that may appear in a generated world's `colors=` list
EMITTABLE = tuple(HUE_NAMES)

# the open-ground color; bright enough to always classify as an empty cell
EMPTY_RGB = (255, 255, 255)

# objects.hpp, with the role each layout wants it for. voxels counts the cluster: the C++
# spawns one WHOLE cluster per solid pixel, so building 4-voxel objects across a dense
# map costs 4x the geometry of the single-voxel ones.
OBJECT_LIBRARY = {
	"tree":     dict(size=(0.5, 1.0, 0.5), voxels=3, roles=("rubble",)),
	"brick":    dict(size=(1.0, 2.0, 1.0), voxels=1, roles=("maze",)),
	"bush":     dict(size=(1.0, 1.0, 1.0), voxels=1, roles=("rubble",)),
	"building": dict(size=(1.0, 2.0, 1.0), voxels=4, roles=("maze",)),
	"rock":     dict(size=(1.5, 1.5, 1.5), voxels=1, roles=("rubble",)),
	"sand":     dict(size=(1.0, 0.5, 1.0), voxels=1, roles=("rubble",)),
	"fence":    dict(size=(0.1, 1.0, 1.0), voxels=1, roles=("maze",)),
	"puddle":   dict(size=(1.0, 1.0, 1.0), voxels=1, roles=("rubble",)),
	"pillar":   dict(size=(0.4, 3.0, 0.4), voxels=1, roles=("columns",)),
	"crate":    dict(size=(0.8, 0.8, 0.8), voxels=1, roles=("columns",)),
}

# the heaviest objects first - a dense world that wants cheap geometry can stop early
OBJECTS_BY_COST = sorted(OBJECT_LIBRARY, key=lambda n: -OBJECT_LIBRARY[n]["voxels"])


def objects_for_role(role: str, max_voxels: int | None = None) -> list[str]:
	"""Object names usable by `role`, cheapest cluster first.

	max_voxels caps the cluster size, which is how a dense world trades look against
	the per-switch rebuild cost without touching its fill target.
	"""
	names = [n for n, d in OBJECT_LIBRARY.items() if role in d["roles"]]
	if max_voxels is not None:
		names = [n for n in names if OBJECT_LIBRARY[n]["voxels"] <= max_voxels]
	return sorted(names, key=lambda n: (OBJECT_LIBRARY[n]["voxels"], n))


def color_hue(r: int, g: int, b: int) -> float:
	"""Hue of an RGB triple in degrees [0, 360)."""
	h, _, _ = colorsys.rgb_to_hsv(r / 255.0, g / 255.0, b / 255.0)
	return h * 360.0


def hue_of(name: str) -> float:
	"""Hue the C++ measures for a palette color."""
	return color_hue(*COLOR_RGB[name])


def is_white(r: int, g: int, b: int) -> bool:
	"""True when a pixel classifies as open ground rather than an object.

	The int() casts matter: numpy hands out uint8 scalars whose sum wraps around, which
	turns a bright pixel into a solid one.
	"""
	return (int(r) + int(g) + int(b)) / 3.0 > WHITE_GRAY_THRESHOLD


def hue_distance(a: float, b: float) -> float:
	"""Circular distance between two hues in degrees (0-180)."""
	d = abs(a - b)
	return min(d, 360.0 - d)


def resolves_to(name: str, enabled) -> str:
	"""Which enabled hue center a pixel of `name` actually builds as.

	Replays getPixelHue's nearest-center search and returns "unknown" past the tolerance.
	"""
	enabled = set(enabled)
	hue = hue_of(name)
	best, best_distance = "unknown", float("inf")
	for center in HUE_ORDER:
		if center not in enabled:
			continue
		distance = hue_distance(hue, HUE_CENTER[center])
		if distance < best_distance:
			best, best_distance = center, distance
	return best if best_distance <= HUE_TOL_DEG else "unknown"


def is_ambiguous(name: str, enabled) -> bool:
	"""True when two enabled centers are exactly as close to `name`'s hue.

	getPixelHue breaks such a tie by iteration order, so a palette with an ambiguous
	color builds differently depending on how the centers were registered. Rejected.
	"""
	hue = hue_of(name)
	distances = sorted(hue_distance(hue, HUE_CENTER[c]) for c in set(enabled) if c in HUE_CENTER)
	return len(distances) >= 2 and distances[0] == distances[1]


def conflicts(names) -> list[str]:
	"""Colors in `names` that would NOT build as themselves with that same set enabled."""
	return [n for n in names if resolves_to(n, names) != n or is_ambiguous(n, names)]


@lru_cache(maxsize=None)
def safe_subsets(n: int) -> tuple[tuple[str, ...], ...]:
	"""Every n-color palette whose members all resolve to themselves, in HUE_ORDER order."""
	out = []
	for combo in combinations(EMITTABLE, n):
		ordered = tuple(c for c in HUE_ORDER if c in combo)
		if len(ordered) != n:
			continue
		if not conflicts(ordered):
			out.append(ordered)
	return tuple(out)


def max_safe_palette() -> int:
	"""Largest palette size that still has a conflict-free combination."""
	return max((n for n in range(1, len(EMITTABLE) + 1) if safe_subsets(n)), default=0)


def parse_colors(names: list[str]) -> list[tuple[str, int, int, int]]:
	"""Resolve a list of color name strings to (name, r, g, b) tuples.

	Skips unknown names with a warning.
	"""
	out = []
	for n in names:
		n = n.strip().lower()
		if n not in COLOR_RGB:
			print(f"palette: unknown color '{n}', skipping")
			continue
		out.append((n, *COLOR_RGB[n]))
	return out