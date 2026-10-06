# obstacles.py - drop colored pixels into the open parts of a canvas.
# Obstacles are solid: the C++ spawns a whole object cluster per non-white pixel, so
# "placing" one means painting a pixel with a palette color, never clearing one.

import numpy as np

from .mapgen import MapCanvas


def _dilate(mask: np.ndarray, steps: int) -> np.ndarray:
	"""Grow a boolean mask `steps` times in 4 directions (Manhattan distance)."""
	out = mask.copy()
	for _ in range(steps):
		nxt = out.copy()
		nxt[1:, :] |= out[:-1, :]
		nxt[:-1, :] |= out[1:, :]
		nxt[:, 1:] |= out[:, :-1]
		nxt[:, :-1] |= out[:, 1:]
		out = nxt
	return out


def free_cells_away_from(mc: MapCanvas, avoid: set[tuple[int, int]], min_dist: int) -> np.ndarray:
	"""Boolean mask of open cells at least `min_dist` Manhattan px from every avoided cell."""
	blocked = np.zeros((mc.height, mc.width), dtype=bool)
	for x, z in avoid:
		if 0 <= x < mc.width and 0 <= z < mc.height:
			blocked[z, x] = True
	return mc.empty & ~_dilate(blocked, min_dist)


def place_around_path(mc: MapCanvas, path_cells: set[tuple[int, int]],
                      color: tuple[int, int, int], count: int = 20,
                      min_dist: int = 3, rng=None) -> list[tuple[int, int]]:
	"""Drop `count` obstacles on open cells at least `min_dist` from any path cell.

	Only free cells are candidates, so obstacles never bury the corridor they decorate.
	Returns the placed positions.
	"""
	if rng is None:
		rng = np.random.default_rng()

	candidates = free_cells_away_from(mc, path_cells, min_dist)
	available = int(candidates.sum())
	if available == 0:
		return []

	placed = []
	for _ in range(min(count, available)):
		ys, xs = np.where(candidates)
		i = int(rng.integers(len(xs)))
		x, z = int(xs[i]), int(ys[i])
		mc.paint(x, z, color)
		placed.append((x, z))
		# a placed obstacle is no longer a candidate
		candidates[ys, xs] = False
		candidates[max(0, z - min_dist):z + min_dist + 1, max(0, x - min_dist):x + min_dist + 1] = False
	return placed


def place_box(mc: MapCanvas, cx: int, cz: int, bw: int, bh: int,
              color: tuple[int, int, int]) -> list[tuple[int, int]]:
	"""Fill a bw x bh rectangle centered at (cx, cz). Returns the occupied cells.

	Only open cells are claimed, so a box laid over existing structure leaves it alone.
	"""
	x0 = max(0, cx - bw // 2)
	z0 = max(0, cz - bh // 2)
	x1 = min(mc.width, x0 + bw)
	z1 = min(mc.height, z0 + bh)

	region = mc.empty[z0:z1, x0:x1]
	region[:] = False
	mc.pixels[z0:z1, x0:x1][region] = color
	return [(x, z) for z in range(z0, z1) for x in range(x0, x1)]


def block_count(mc: MapCanvas) -> int:
	"""Number of connected solid components - a cheap 'is this one blob' sanity check."""
	seen = np.zeros((mc.height, mc.width), dtype=bool)
	solid = ~mc.empty
	components = 0
	for z0, x0 in zip(*np.where(solid & ~seen)):
		if seen[z0, x0]:
			continue
		components += 1
		stack = [(int(z0), int(x0))]
		seen[z0, x0] = True
		while stack:
			z, x = stack.pop()
			for nz, nx in ((z + 1, x), (z - 1, x), (z, x + 1), (z, x - 1)):
				if 0 <= nz < mc.height and 0 <= nx < mc.width and solid[nz, nx] and not seen[nz, nx]:
					seen[nz, nx] = True
					stack.append((nz, nx))
	return components