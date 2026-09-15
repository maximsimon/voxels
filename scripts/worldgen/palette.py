# RGB palette that mirrors C++ hue types; each color's hue sits inside the 45-degree tolerance.
# avg RGB > 150 = empty (bright pixels ignored by the voxel builder).
# use parse_colors(["red","green"]) to get [(HUE_TYPE, r, g, b), ...] for registry writers.

import colorsys

# (name, r, g, b) - all verified hue-safe
_COLOR_TABLE = [
	("red",    180,  0,  0),
	("orange", 200, 128,  0),
	("yellow", 180, 180,  0),
	("green",    0, 180,  0),
	("cyan",     0, 180, 180),
	("blue",     0,   0, 180),
	("purple",  128,  0, 180),
	("pink",   200, 100, 140),
	("brown",   140, 80, 20),
	("white",   255, 255, 255),
	("black",    20,  20,  20),
]

COLOR_RGB = {name: (r, g, b) for name, r, g, b in _COLOR_TABLE}

# display names for registry files (the C++ hue_by_name accepts lowercase names)
HUE_NAMES = [name for name, _, _, _ in _COLOR_TABLE]


def color_hue(r: int, g: int, b: int) -> float:
	"""Hue of an RGB triple in degrees [0, 360)."""
	h, _, _ = colorsys.rgb_to_hsv(r / 255.0, g / 255.0, b / 255.0)
	return h * 360.0


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
