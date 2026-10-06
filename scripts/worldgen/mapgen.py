# mapgen.py - MapCanvas, the pixel buffer every generator draws into.
# map sides are multiples of 16 (CHUNK_WIDTH=16); world coord == pixel coord.
# bright pixels (avg RGB > 150, mirroring the C++ white_gray_threshold) = open ground;
# colored pixels = solid voxels. Colors come from palette.py so getPixelHue accepts them.

from pathlib import Path

import numpy as np
from PIL import Image

from .palette import EMPTY_RGB, is_white

CHUNK_WIDTH = 16


def _flood(mask: np.ndarray, passable: np.ndarray, limit: int = 4096) -> np.ndarray:
	"""Grow `mask` by repeated 4-neighbour dilation until it stops changing.

	Whole-array numpy ops instead of a per-cell queue: a 256x256 map converges in a few
	hundred iterations at well under a tenth of a second, which is what makes connectivity
	affordable to check on every generated world. `passable` is re-applied every step so the
	flood cannot bleed through solid pixels.
	"""
	out = mask & passable
	for _ in range(limit):
		nxt = out.copy()
		nxt[1:, :] |= out[:-1, :]
		nxt[:-1, :] |= out[1:, :]
		nxt[:, 1:] |= out[:, :-1]
		nxt[:, :-1] |= out[:, 1:]
		nxt &= passable
		if nxt.sum() == out.sum():
			break
		out = nxt
	return out


def reachable_from_origin(mc: "MapCanvas", start: tuple[int, int] = (0, 0)) -> tuple[np.ndarray, float]:
	"""Flood the open ground from `start`; return the mask and the reachable free fraction.

	The origin is where the trainer spawns, and negative coords fall off the map and read as
	open, so flooding from (0,0) also covers the off-map apron the robot starts in.
	"""
	free = mc.empty
	if not (0 <= start[0] < mc.width and 0 <= start[1] < mc.height) or not free[start[1], start[0]]:
		return np.zeros_like(free), 0.0
	seed = np.zeros_like(free)
	seed[start[1], start[0]] = True
	seen = _flood(seed, free)
	return seen, float(seen.sum()) / max(1, int(free.sum()))


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
		# cells no layout may fill: the trainer spawns robots at random poses within
		# spawn_radius of the origin, and coords below 0 fall off the map entirely
		self.protected = np.zeros((height, width), dtype=bool)

	def reserve_spawn(self, radius: int):
		"""Keep a disc of open ground around the origin clear of obstacles."""
		ys, xs = np.ogrid[:self.height, :self.width]
		self.protected |= ((xs - 0.5) ** 2 + (ys - 0.5) ** 2) <= radius * radius

	def reserve(self, mask: np.ndarray):
		self.protected |= mask

	@property
	def paintable(self) -> np.ndarray:
		"""Open cells a layout is allowed to claim."""
		return self.empty & ~self.protected

	# ------------------------------------------------------------------
	# painting
	# ------------------------------------------------------------------

	def paint(self, x: int, z: int, color: tuple[int, int, int], empty: bool | None = None):
		"""Set one pixel.

		Emptiness is derived from the color's brightness unless `empty` overrides it,
		which is what carving wants: pathgen paints a corridor in a wall color but keeps
		the cells walkable.
		"""
		if 0 <= x < self.width and 0 <= z < self.height:
			self.pixels[z, x] = color
			self.empty[z, x] = self._empty_for(color) if empty is None else empty

	def rect(self, x0: int, z0: int, x1: int, z1: int, color: tuple[int, int, int],
	         empty: bool | None = None):
		"""Fill a rectangle [x0, x1) x [z0, z1)."""
		x0 = max(0, min(x0, self.width))
		z0 = max(0, min(z0, self.height))
		x1 = max(0, min(x1, self.width))
		z1 = max(0, min(z1, self.height))
		self.pixels[z0:z1, x0:x1] = color
		self.empty[z0:z1, x0:x1] = self._empty_for(color) if empty is None else empty

	def paint_mask(self, mask: np.ndarray, color: tuple[int, int, int]):
		"""Paint every cell where `mask` is True. Bulk walls, in one numpy write."""
		sel = mask & ~self.empty
		self.pixels[sel] = color

	def line(self, x0: int, z0: int, x1: int, z1: int, color: tuple[int, int, int],
	         empty: bool | None = None):
		"""Draw a 1px Bresenham line."""
		dx = abs(x1 - x0)
		dz = abs(z1 - z0)
		sx = 1 if x1 >= x0 else -1
		sz = 1 if z1 >= z0 else -1
		x, z = x0, z0
		err = dx - dz
		while True:
			self.paint(x, z, color, empty=empty)
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

	def clear_rect(self, x0: int, z0: int, x1: int, z1: int):
		"""Cut a rectangle back to open ground."""
		self.rect(x0, z0, x1, z1, EMPTY_RGB)

	def clear_protected(self):
		"""Force every reserved cell back to open ground."""
		self.pixels[self.protected] = EMPTY_RGB
		self.empty[self.protected] = True

	# ------------------------------------------------------------------
	# queries
	# ------------------------------------------------------------------

	@staticmethod
	def _empty_for(color) -> bool:
		return is_white(*color)

	def is_empty(self, x: int, z: int) -> bool:
		if 0 <= x < self.width and 0 <= z < self.height:
			return bool(self.empty[z, x])
		return True

	def is_solid(self, x: int, z: int) -> bool:
		return not self.is_empty(x, z)

	def free_cells(self) -> list[tuple[int, int]]:
		"""Return [(x, z), ...] for every open pixel."""
		ys, xs = np.where(self.empty)
		return list(zip(xs.tolist(), ys.tolist()))

	def white_fraction(self) -> float:
		"""Fraction of open pixels - the classic maps are 74-97% white."""
		return float(self.empty.mean())

	def fill_fraction(self) -> float:
		"""Fraction of solid pixels. This is the number every layout tunes against."""
		return 1.0 - self.white_fraction()

	def fill_to(self, target: float) -> bool:
		"""True once the canvas has reached `target` solid fraction."""
		return self.fill_fraction() >= target

	# ------------------------------------------------------------------
	# PNG I/O
	# ------------------------------------------------------------------

	def save_png(self, path: str | Path):
		Image.fromarray(self.pixels).save(str(path))
		print(f"mapgen: saved {path} ({self.width}x{self.height}, "
		      f"{self.white_fraction():.0%} open, {self.fill_fraction():.1%} solid)")

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
		mc.empty = np.array([cls._empty_for(c) for c in arr.reshape(-1, 3)]).reshape(h, w)
		return mc