# pathgen.py - carve a walkable corridor through a MapCanvas.
#
# the trap this module exists to avoid: the PNG is what the C++ reads, not our python-side
# emptiness flag. Painting a corridor with a dark palette color and marking it empty=True
# still saves a DARK pixel, and getPixelHue builds a solid object there. A corridor is
# carved with white, which is the only thing that survives both sides.

from .mapgen import MapCanvas
from .palette import EMPTY_RGB


def draw_path(mc: MapCanvas, waypoints: list[tuple[int, int]], width: int = 1,
              color: tuple[int, int, int] | None = None) -> set[tuple[int, int]]:
	"""Carve an open corridor along `waypoints`, each a pixel (x, z).

	`width` is the half-width in pixels (1 = a 3px-wide corridor). Waypoints are connected
	in order using MapCanvas.line's Bresenham. Pass `color` to paint the corridor in a
	palette color while leaving it walkable - only useful for maps that are read back with
	MapCanvas.from_png, never for a map handed to the simulator.

	Returns the set of center-line cells, which obstacles.py uses to keep clutter off the
	corridor.
	"""
	path_cells: set[tuple[int, int]] = set()
	half = max(0, width)
	for i in range(len(waypoints) - 1):
		x0, z0 = waypoints[i]
		x1, z1 = waypoints[i + 1]
		# thicker than 1px: draw parallel offsets so the corridor stays even width on
		# both axis steps instead of only where the line happens to run diagonal.
		for off in range(-half, half + 1):
			mc.line(x0, z0 + off, x1, z1 + off, color or EMPTY_RGB, empty=True)
			mc.line(x0 + off, z0, x1 + off, z1, color or EMPTY_RGB, empty=True)
		# collect center-line cells from the Bresenham path itself
		dx = abs(x1 - x0)
		dz = abs(z1 - z0)
		sx = 1 if x1 >= x0 else -1
		sz = 1 if z1 >= z0 else -1
		x, z = x0, z0
		err = dx - dz
		while True:
			path_cells.add((x, z))
			if x == x1 and z == z1:
				break
			e2 = 2 * err
			if e2 > -dz:
				err -= dz
				x += sx
			if e2 < dx:
				err += dx
				z += sz
	return path_cells