# visualize_trail.py - scatter natural obstacles around a driven path for RL avoidance training.
# Reads a trail CSV written by play.py, loads the map image recorded in its header (or the one
# given with --map), and saves a copy of the map with obstacles placed along the corridor the
# robot drove: some cells of the path itself become blocked, some nearby cells become blocked,
# with gaps left everywhere else so a robot can still navigate through.
#
# Usage:
#   python scripts/visualize_trail.py <trail.csv> [--map PATH] [--out PATH] [--color NAME]
#                                             [--spacing N] [--saturation F] [--on-fraction F]
#                                             [--near-radius N] [--seed N] [--show]
#
# Obstacles use a color from the world's palette (the color coding the world config used when
# the trail was recorded), so the simulator recognizes the new pixels as solid voxels when the
# modified map is loaded back as a world.

import argparse
import csv
import os
import random
import sys

import cv2
import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from worldgen.palette import COLOR_RGB

DEFAULT_COLORS = ["red", "green", "blue", "pink"]

# small shapes a node can take - weights make single blocks most common, doubles and
# L-shapes rarer so the field does not look machine-placed
_PATTERNS = [
	([(0, 0)], 5),
	([(0, 0), (1, 0)], 2),
	([(0, 0), (0, 1)], 2),
	([(0, 0), (1, 0), (0, 1)], 1),
]


def _read_trail(trail_path):
	meta = {}
	points = []
	with open(trail_path, "r", newline="") as f:
		reader = csv.reader(f)
		for row in reader:
			if not row:
				continue
			if row[0] == "step":
				continue
			if row[0].startswith("#"):
				line = ",".join(row)
				if line.startswith("# "):
					key, _, value = line[2:].partition("=")
					meta[key] = value.strip()
				continue
			points.append((float(row[1]), float(row[3])))
	if not points:
		raise SystemExit("no trail points in %s" % trail_path)
	return meta, points


def _ordered_path_cells(points, width, height):
	order = []
	seen = set()
	for x, z in points:
		px = int(round(x))
		pz = int(round(z))
		if px < 0 or pz < 0 or px >= width or pz >= height:
			continue
		cell = (px, pz)
		if cell in seen:
			continue
		seen.add(cell)
		order.append(cell)
	return order


def main():
	ap = argparse.ArgumentParser()
	ap.add_argument("trail", help="trail CSV written by play.py")
	ap.add_argument("--map", default=None, help="map image to place obstacles onto (defaults to the one recorded in the trail header)")
	ap.add_argument("--out", default=None, help="output PNG path (defaults to the trail file name with _obstacles appended)")
	ap.add_argument("--color", default=None, help="color name for the obstacle pixels (defaults to the first color of the world's palette)")
	ap.add_argument("--spacing", type=int, default=6, help="average path cells between placement attempts")
	ap.add_argument("--saturation", type=float, default=0.75, help="probability that an attempt actually places an obstacle")
	ap.add_argument("--on-fraction", type=float, default=0.35, help="fraction of placed obstacles on the path itself (rest sit beside it)")
	ap.add_argument("--near-radius", type=int, default=3, help="max distance in cells of alongside obstacles from the path")
	ap.add_argument("--seed", type=int, default=None, help="RNG seed for reproducible placement")
	ap.add_argument("--show", action="store_true", help="show the result in a window and wait for a key")
	args = ap.parse_args()

	meta, points = _read_trail(args.trail)
	map_path = args.map or meta.get("map")
	if not map_path:
		raise SystemExit("no map image recorded in the trail header - pass one with --map")

	palette = [c.strip() for c in meta.get("colors", "").split(",") if c.strip()] or DEFAULT_COLORS
	for c in palette:
		if c not in COLOR_RGB:
			raise SystemExit("world palette contains unknown color '%s'" % c)

	color_name = args.color or palette[0]
	if color_name not in COLOR_RGB:
		raise SystemExit("unknown color '%s' - known colors: %s" % (color_name, ", ".join(COLOR_RGB)))
	if color_name not in palette:
		print("warning: '%s' is not in the world palette %s" % (color_name, palette))

	img = cv2.imread(map_path, cv2.IMREAD_UNCHANGED)
	if img is None:
		raise SystemExit("could not load map image '%s'" % map_path)

	canvas = img.copy()
	if canvas.shape[2] == 4:
		canvas = cv2.cvtColor(canvas, cv2.COLOR_BGRA2BGR)
	r, g, b = COLOR_RGB[color_name]
	color_bgr = (b, g, r)

	height, width = canvas.shape[:2]
	order = _ordered_path_cells(points, width, height)
	if not order:
		raise SystemExit("no trail points fall inside the map")

	path_mask = np.zeros((height, width), np.uint8)
	for px, pz in order:
		path_mask[pz, px] = 1
	distance_to_path = cv2.distanceTransform(1 - path_mask, cv2.DIST_L2, 3)

	rng = random.Random(args.seed)
	weighted = [p for p, _ in _PATTERNS]
	weights = [w for _, w in _PATTERNS]
	obstacles = set()
	on_path = 0
	off_path = 0

	def open_cell(px, pz):
		if not (0 <= px < width and 0 <= pz < height):
			return False
		if (px, pz) in obstacles:
			return False
		c = canvas[pz, px]
		return (int(c[0]) + int(c[1]) + int(c[2])) / 3 > 150

	def cluster_ok(cells):
		for px, pz in cells:
			if not open_cell(px, pz):
				return False
			for dx in (-1, 0, 1):
				for dz in (-1, 0, 1):
					if (dx == 0) == (dz == 0):
						continue
					if (px + dx, pz + dz) in obstacles:
						return False
		return True

	def near_cell(px, pz):
		for _ in range(30):
			dx = rng.randint(-args.near_radius, args.near_radius)
			dz = rng.randint(-args.near_radius, args.near_radius)
			if dx == 0 and dz == 0:
				continue
			qx, qz = px + dx, pz + dz
			if not (0 <= qx < width and 0 <= qz < height):
				continue
			d = distance_to_path[qz, qx]
			if 1.0 <= d <= args.near_radius:
				return qx, qz
		return None

	def paint(cells):
		for px, pz in cells:
			canvas[pz, px] = color_bgr
			obstacles.add((px, pz))

	i = 0
	while i < len(order):
		if rng.random() < args.saturation:
			px, pz = order[i]
			center = (px, pz)
			on_the_path = rng.random() < args.on_fraction
			if not on_the_path:
				candidate = near_cell(px, pz)
				if candidate is None:
					i += rng.randint(1, max(1, args.spacing * 2))
					continue
				center = candidate
			shape = rng.choices(weighted, weights=weights, k=1)[0]
			cells = [(center[0] + dx, center[1] + dz) for dx, dz in shape]
			if cluster_ok(cells):
				paint(cells)
				if on_the_path:
					on_path += 1
				else:
					off_path += 1
		i += rng.randint(1, max(1, args.spacing * 2))

	out_path = args.out or os.path.splitext(args.trail)[0] + "_obstacles.png"
	cv2.imwrite(out_path, canvas)
	print("world        :", meta.get("world", "?"))
	print("map          :", map_path)
	print("palette      :", palette)
	print("obstacle color:", color_name, COLOR_RGB[color_name])
	print("path cells   :", len(order))
	print("obstacles    :", on_path, "on path,", off_path, "beside path (total", len(obstacles), "cells)")
	on_path_cells = len(set(order) & obstacles)
	open_fraction = 100.0 * (len(order) - on_path_cells) / max(1, len(order))
	print("path open    : %.0f%% of path cells left empty" % open_fraction)
	print("saved        :", out_path)

	if args.show:
		tag = "obstacles (%d cells, color %s) - press any key" % (len(obstacles), color_name)
		cv2.imshow(tag, canvas)
		cv2.waitKey(0)
		cv2.destroyAllWindows()


if __name__ == "__main__":
	main()