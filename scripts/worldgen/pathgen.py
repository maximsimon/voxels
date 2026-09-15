# draw_path: carve a walkable corridor on a MapCanvas from a list of waypoints.
# returns the set of free (non-empty) cells along the path for obstacle placement.

from .mapgen import MapCanvas


def draw_path(mc: MapCanvas, waypoints: list[tuple[int, int]], width: int = 1,
              color: tuple[int, int, int] = (20, 20, 20)) -> set[tuple[int, int]]:
	"""Carve a corridor along `waypoints`, each a pixel (x, z).

	`width` is half-width in pixels (1 = 3px wide: center + 1 each side).
	waypoints are connected in order; the path is carved as empty.
	Returns the set of center-line cells for obstacle placement.
	"""
	path_cells: set[tuple[int, int]] = set()
	for i in range(len(waypoints) - 1):
		x0, z0 = waypoints[i]
		x1, z1 = waypoints[i + 1]
		# Bresenham-ish step along the longer axis
		dx = abs(x1 - x0)
		dz = abs(z1 - z0)
		sx = 1 if x1 >= x0 else -1
		sz = 1 if z1 >= z0 else -1
		x, z = x0, z0
		if dx >= dz:
			while True:
				for dz_off in range(-width, width + 1):
					mc.paint(x, z + dz_off, color, empty=True)
				path_cells.add((x, z))
				if x == x1:
					break
				x += sx
				err = dx - dz
				if err >= 0:
					z += sz
					dz += 2
				dx -= 2
		else:
			while True:
				for dx_off in range(-width, width + 1):
					mc.paint(x + dx_off, z, color, empty=True)
				path_cells.add((x, z))
				if z == z1:
					break
				z += sz
				err = dz - dx
				if err >= 0:
					x += sx
					dx += 2
				dz -= 2
	return path_cells
