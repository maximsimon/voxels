# map generator: MapCanvas, variant helpers, PNG I/O
# map sides are multiples of 16 (CHUNK_WIDTH=16); world coord == pixel coord.
# bright pixels (avg RGB > 150) = empty/open ground; colored pixels = solid voxels.
# colors come from the C++ palette (see palette.py) so the voxel builder can render them.

from pathlib import Path
import numpy as np
from PIL import Image

from .palette import COLOR_RGB

CHUNK_WIDTH = 16
EMPTY_THRESHOLD = 150  # avg RGB above this = empty


def _is_bright(color: tuple[int, int, int]) -> bool:
	return sum(color) / 3.0 > EMPTY_THRESHOLD


class MapCanvas:
	"""A 2D pixel map where open ground is white and walls/obstacles are palette colors."""

	def __init__(self, width: int, height: int):
		assert width % CHUNK_WIDTH == 0 and height % CHUNK_WIDTH == 0, \
			"map sides must be multiples of 16"
		self.width = width
		self.height = height
		# all-white canvas: fully open until walls are painted
		self.pixels = np.full((height, width, 3), 255, dtype=np.uint8)
		self.empty = np.ones((height, width), dtype=bool)

	# ------------------------------------------------------------------
	# painting
	# ------------------------------------------------------------------

	def paint(self, x: int, z: int, color: tuple[int, int, int]):
		"""Set one pixel; emptiness is derived from its brightness."""
		if 0 <= x < self.width and 0 <= z < self.height:
			self.pixels[z, x] = color
			self.empty[z, x] = not _is_bright(color)

	def rect(self, x0: int, z0: int, x1: int, z1: int, color: tuple[int, int, int]):
		"""Fill a rectangle [x0, x1) x [z0, z1)."""
		x0 = max(0, min(x0, self.width))
		z0 = max(0, min(z0, self.height))
		x1 = max(0, min(x1, self.width))
		z1 = max(0, min(z1, self.height))
		self.pixels[z0:z1, x0:x1] = color
		self.empty[z0:z1, x0:x1] = not _is_bright(color)

	def line(self, x0: int, z0: int, x1: int, z1: int, color: tuple[int, int, int]):
		"""Draw a 1px Bresenham line."""
		dx = abs(x1 - x0)
		dz = abs(z1 - z0)
		sx = 1 if x1 >= x0 else -1
		sz = 1 if z1 >= z0 else -1
		x, z = x0, z0
		err = dx - dz
		while True:
			self.paint(x, z, color)
			if x == x1 and z == z1:
				break
			e2 = 2 * err
			if e2 > -dz:
				err -= dz
				x += sx
			if e2 < dx:
				err += dx
				z += sz

	def fill_border(self, color: tuple[int, int, int], thickness: int = 2):
		"""Paint a solid border around the whole map."""
		self.rect(0, 0, self.width, thickness, color)
		self.rect(0, self.height - thickness, self.width, self.height, color)
		self.rect(0, 0, thickness, self.height, color)
		self.rect(self.width - thickness, 0, self.width, self.height, color)

	# ------------------------------------------------------------------
	# queries
	# ------------------------------------------------------------------

	def is_empty(self, x: int, z: int) -> bool:
		if 0 <= x < self.width and 0 <= z < self.height:
			return self.empty[z, x]
		return True

	def is_solid(self, x: int, z: int) -> bool:
		return not self.is_empty(x, z)

	def free_cells(self) -> list[tuple[int, int]]:
		"""Return [(x, z), ...] for every open pixel."""
		ys, xs = np.where(self.empty)
		return list(zip(xs.tolist(), ys.tolist()))

	def white_fraction(self) -> float:
		"""Fraction of open pixels - the classic maps are 85-97% white."""
		return float(self.empty.mean())

	# ------------------------------------------------------------------
	# PNG I/O
	# ------------------------------------------------------------------

	def save_png(self, path: str | Path):
		Image.fromarray(self.pixels).save(str(path))
		print(f"mapgen: saved {path} ({self.width}x{self.height}, {self.white_fraction():.0%} open)")

	@classmethod
	def from_png(cls, path: str | Path) -> "MapCanvas":
		"""Load a PNG map. Bright pixels (avg > threshold) become empty."""
		img = Image.open(path).convert("RGB")
		arr = np.array(img)
		h, w = arr.shape[:2]
		mc = cls.__new__(cls)
		mc.width = w
		mc.height = h
		mc.pixels = arr
		mc.empty = arr.mean(axis=2) > EMPTY_THRESHOLD
		return mc


# ------------------------------------------------------------------
# variant generator - map_paper style: white open ground, colored walls
# ------------------------------------------------------------------

def _place_walled_room(mc: MapCanvas, x0: int, z0: int, w: int, h: int,
                       color: tuple[int, int, int], thickness: int = 2,
                       rng=None) -> None:
	"""Draw a hollow room outline with one random door gap per wall.

	Walls are `thickness` px; each of the four sides keeps a door gap
	open so the room stays reachable and rooms connect freely.
	"""
	if rng is None:
		rng = np.random.default_rng()
	x0 = max(0, x0)
	z0 = max(0, z0)
	x1 = min(mc.width, x0 + w)
	z1 = min(mc.height, z0 + h)
	if x1 - x0 <= 2 * thickness + 2 or z1 - z0 <= 2 * thickness + 2:
		return

	# top wall with a door gap in the middle-ish zone
	door_w = max(3, (x1 - x0) // 6)
	left = mc.width // 2 - door_w // 2
	right = left + door_w
	mc.rect(x0 + thickness, z0, x1 - thickness, z0 + thickness, color)
	mc.rect(x0 + thickness, z0, left - thickness, z0 + thickness, (255, 255, 255))
	mc.rect(right + thickness, z0, x1 - thickness, z0 + thickness, (255, 255, 255))

	# bottom wall
	mc.rect(x0 + thickness, z1 - thickness, x1 - thickness, z1, color)
	mc.rect(x0 + thickness, z1 - thickness, left - thickness, z1, (255, 255, 255))
	mc.rect(right + thickness, z1 - thickness, x1 - thickness, z1, (255, 255, 255))

	# left wall (door on a random interior row)
	door_h = max(3, (z1 - z0) // 6)
	top = mc.height // 2 - door_h // 2
	bottom = top + door_h
	mc.rect(x0, z0 + thickness, x0 + thickness, z1 - thickness, color)
	mc.rect(x0, z0 + thickness, x0 + thickness, top, (255, 255, 255))
	mc.rect(x0, bottom, x0 + thickness, z1 - thickness, (255, 255, 255))

	# right wall
	mc.rect(x1 - thickness, z0 + thickness, x1, z1 - thickness, color)
	mc.rect(x1 - thickness, z0 + thickness, x1, top, (255, 255, 255))
	mc.rect(x1 - thickness, bottom, x1, z1 - thickness, (255, 255, 255))


def generate_variant(width: int, height: int, rooms: int = 4,
                     colors: list[tuple[int, int, int]] | None = None,
                     obstacles: int = 12, seed: int | None = None) -> MapCanvas:
	"""Generate a map_paper-style variant: white open ground + colored room walls.

	width and height are in pixels and must be multiples of 16.
	`rooms` hollow colored rectangles get a door gap on every side; `obstacles`
	small colored blocks are scattered in the open areas. Returns the MapCanvas.
	"""
	rng = np.random.default_rng(seed)
	mc = MapCanvas(width, height)
	palette = colors or list(COLOR_RGB.values())

	# NOTE: no enclosing border - the robot auto-spawns just outside the map corner
	# and drives in; a rim wall at the edge would trap it before it ever enters.

	# rooms: hollow rects inside the map, door gaps on all four sides
	wall_colors = palette[1:4] or palette
	for i in range(rooms):
		w = int(rng.integers(20, max(21, width // 2)))
		h = int(rng.integers(20, max(21, height // 2)))
		x0 = int(rng.integers(4, max(5, width - w - 4)))
		z0 = int(rng.integers(4, max(5, height - h - 4)))
		_place_walled_room(mc, x0, z0, w, h, wall_colors[i % len(wall_colors)], rng=rng)

	# scattered obstacle blocks (trees/bricks/buildings) in the open areas
	open_cells = mc.free_cells()
	if open_cells:
		obstacle_color = palette[1:]
		for i in range(obstacles):
			if not open_cells:
				break
			cx, cz = open_cells[rng.integers(len(open_cells))]
			# keep a little padding so blocks do not merge into walls
			mc.rect(cx, cz, cx + 2, cz + 2, obstacle_color[rng.integers(len(obstacle_color))])
			open_cells = [p for p in open_cells if abs(p[0] - cx) > 3 or abs(p[1] - cz) > 3]

	return mc