# place obstacles around a path or in arbitrary free cells.
# obstacles are colored pixels; empty path cells are avoided.

from .mapgen import MapCanvas


def place_around_path(mc: MapCanvas, path_cells: set[tuple[int, int]],
                      color: tuple[int, int, int], count: int = 20,
                      min_dist: int = 3, rng=None) -> list[tuple[int, int]]:
	"""Drop `count` obstacles on free cells near (but not on) the path.

	min_dist is minimum Manhattan distance from any path cell.
	obstacles are placed on the border or on solid cells adjacent to empty ones
	so they have a visible surface. Returns the list of placed positions.
	"""
	import numpy as np
	if rng is None:
		rng = np.random.default_rng()

	free = mc.free_cells()
	# cells that are empty and away from the path
	candidates = [(x, z) for x, z in free
	              if not mc.is_empty(x, z)
	              and all(abs(x - px) + abs(z - pz) > min_dist for px, pz in path_cells)]

	if not candidates:
		return []

	placed = []
	for _ in range(count):
		if not candidates:
			break
		idx = rng.integers(len(candidates))
		x, z = candidates.pop(idx)
		mc.paint(x, z, color)
		placed.append((x, z))
	return placed


def place_box(mc: MapCanvas, cx: int, cz: int, bw: int, bh: int,
              color: tuple[int, int, int]) -> list[tuple[int, int]]:
	"""Place a rectangular obstacle centered at (cx, cz). Returns occupied cells."""
	placed = []
	for dx in range(-(bw // 2), bw // 2 + 1):
		for dz in range(-(bh // 2), bh // 2 + 1):
			x, z = cx + dx, cz + dz
			if not mc.is_empty(x, z):
				mc.paint(x, z, color)
				placed.append((x, z))
	return placed
