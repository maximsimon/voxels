# layouts.py - build a map by running one or more generators at a shared fill target.
#
# every generator has the same shape and fills to an ABSOLUTE solid fraction, so layout and
# density are independent axes: generate_map() runs the chosen layouts in order and hands
# each one the cumulative target `target_fill * (i + 1) / n`. Because a generator only ever
# ADDS solid pixels, the last one lands the canvas inside its density band regardless of
# which layouts were combined.
#
# what the map image controls is how MANY object instances appear and where; the C++ object
# definition controls their shape (map.cpp::genObject spawns one whole cluster per non-white
# pixel). That is why a single pixel in the `columns` layout can be a 3-unit-tall pillar
# while a 2x2 block of `rubble` is four boulders.

from __future__ import annotations

import numpy as np

from .mapgen import MapCanvas, reachable_from_origin
from .palette import COLOR_RGB, EMPTY_RGB

# solid-pixel bands, anchored to the hand-made maps already in resources/worlds:
# map_paper 3.15% (ConferenceWorld), mazemap_128 12.5%, maze_picture 26.5%
DENSITY_TIERS = {
	"sparse": (0.03, 0.06),
	"medium": (0.12, 0.18),
	"dense": (0.20, 0.32),
}
TIER_ORDER = ["sparse", "medium", "dense"]

# the trainer spawns inside +-spawn_radius (6) of the origin, so keep a wider disc clear
SPAWN_CLEAR_RADIUS = 10

# open ground has to stay reachable from the spawn, or the map looks fine and plays as a cell
MIN_REACHABLE = 0.55

# how far past `budget` a lattice's ceiling must reach to be preferred. the lattice only
# offers a few discrete cell sizes, so this margin is what lets the braid tune the final
# percent or so of density instead of the cell size having to land on it exactly
MAZE_CEILING_MARGIN = 1.02

# the narrowest corridor a maze may leave. one pixel is technically drivable but the robot
# scrapes both walls, and two-pixel gaps already read as noise rather than as a passage
MAZE_MIN_CORRIDOR = 2

LAYOUTS: dict[str, callable] = {}
LAYOUT_ROLES: dict[str, str] = {}


def layout(name: str):
	"""Register a generator under `name`."""
	def wrap(fn):
		LAYOUTS[name] = fn
		LAYOUT_ROLES[name] = fn.__name__.lstrip("_")
		return fn
	return wrap


def tier_bounds(tier: str) -> tuple[float, float]:
	if tier not in DENSITY_TIERS:
		raise ValueError(f"unknown density tier '{tier}' - known: {', '.join(TIER_ORDER)}")
	return DENSITY_TIERS[tier]


# ------------------------------------------------------------------
# shared placement helpers
# ------------------------------------------------------------------

def _claim(rng: np.random.Generator, allowed: np.ndarray,
           anchors: list[tuple[int, int]], reach: int,
           clump: float) -> tuple[int, int] | None:
	"""Take one cell out of `allowed`, preferring to cluster near an existing element.

	Falls back to the whole field when the clump window is exhausted, and returns None only
	when the field itself is full.
	"""
	if anchors and rng.random() < clump:
		ax, az = anchors[int(rng.integers(len(anchors)))]
		window = np.zeros_like(allowed)
		y0, y1 = max(0, az - reach), min(allowed.shape[0], az + reach + 1)
		x0, x1 = max(0, ax - reach), min(allowed.shape[1], ax + reach + 1)
		window[y0:y1, x0:x1] = allowed[y0:y1, x0:x1]
		picked = _first_cell(rng, window)
		if picked is not None:
			return picked
	return _first_cell(rng, allowed)


def _first_cell(rng: np.random.Generator, mask: np.ndarray) -> tuple[int, int] | None:
	ys, xs = np.where(mask)
	if len(xs) == 0:
		return None
	i = int(rng.integers(len(xs)))
	return int(xs[i]), int(ys[i])


def _picker(rng: np.random.Generator, style: list[tuple[tuple[int, int, int], str]]):
	"""Round-robin over a layout's style list, from a random start.

	Round-robin rather than per-element random so every declared color is guaranteed to be
	drawn once the layout has placed len(style) elements - a purely random pick can leave a
	color out on a sparse map, and an unused color in the palette is a lie in the registry.
	"""
	idx = int(rng.integers(len(style))) if len(style) > 1 else 0

	def pick() -> tuple[tuple[int, int, int], str]:
		nonlocal idx
		entry = style[idx % len(style)]
		idx += 1
		return entry

	return pick


def _open_ring(allowed: np.ndarray, x: int, z: int, radius: int,
               width: int = 1, height: int = 1):
	"""Blank a square around (x, z) so a cluster cannot fuse with its neighbour."""
	y0, y1 = max(0, z - radius), min(allowed.shape[0], z + radius + height)
	x0, x1 = max(0, x - radius), min(allowed.shape[1], x + radius + width)
	allowed[y0:y1, x0:x1] = False





# ------------------------------------------------------------------
# maze: corridors on a grid, reads as indoor
# ------------------------------------------------------------------

def _carve_grid(cols: int, rows: int, rng: np.random.Generator,
                braid: float) -> tuple[set, set, set]:
	"""Randomized-DFS maze walls.

	vw holds vertical walls left of cell column i, hw horizontal walls above cell row j, and
	braid holds the extra edges `braid` knocked out to give the maze loops.

	These are all NON-tree edges: the tree edges were carved open during the DFS and are what
	make the corridors a connected network. They must never be re-closed - doing so walls the
	spawn area into a pocket - which is why the braid-opened edges are returned separately
	instead of being inferred from "not a wall".
	"""
	vw = {(i, j) for i in range(1, cols) for j in range(rows)}
	hw = {(i, j) for i in range(cols) for j in range(1, rows)}

	visited = {(0, 0)}
	stack = [(0, 0)]
	while stack:
		ci, cj = stack[-1]
		options = []
		if ci > 0 and (ci - 1, cj) not in visited:
			options.append(((ci - 1, cj), "v", (ci, cj)))
		if ci < cols - 1 and (ci + 1, cj) not in visited:
			options.append(((ci + 1, cj), "v", (ci + 1, cj)))
		if cj > 0 and (ci, cj - 1) not in visited:
			options.append(((ci, cj - 1), "h", (ci, cj)))
		if cj < rows - 1 and (ci, cj + 1) not in visited:
			options.append(((ci, cj + 1), "h", (ci, cj + 1)))
		if not options:
			stack.pop()
			continue
		nxt, axis, wall = options[int(rng.integers(len(options)))]
		(vw if axis == "v" else hw).discard(wall)
		visited.add(nxt)
		stack.append(nxt)

	braid_opened: set = set()
	for walls, axis in ((vw, "v"), (hw, "h")):
		for _ in range(int(len(walls) * braid)):
			if not walls:
				break
			i, j = walls.pop()
			braid_opened.add((axis, i, j))
	return vw, hw, braid_opened


def _lattice_ceiling(mc: MapCanvas, cell: int, thickness: int, cols: int,
                     rows: int) -> float:
	"""Share of the map a lattice would wall if every non-tree edge were a wall.

	Measured against the ACTUAL lattice rather than the analytic 1/cell, for two reasons that
	matter at the dense end: `width // cell` leaves a remainder so the lattice does not span
	the map, and the non-tree edges are (cols*rows - cols - rows + 1), not cols*rows - 1. The
	analytic form overstates cell 3 by ~2%, which is more than the whole margin a 32% target
	has to spare, so using it silently makes the densest tiers unreachable.
	"""
	total = float(mc.width * mc.height)
	non_tree = cols * rows - cols - rows + 1
	lattice_area = (cols * cell) * (rows * cell)
	return non_tree * cell * thickness / total * (lattice_area / total)


def _fit_lattice(mc: MapCanvas, budget: float) -> tuple[int, int]:
	"""Pick the (cell, thickness) lattice that covers `budget` with the widest corridor.

	A maze's wall share is bounded by (cell - thickness) corridors at `thickness` each, so the
	corridor width is what a density target actually competes with: the densest tier cannot be
	reached by shrinking cells alone, only by thickening walls until wall area closes the gap.
	Thin walls are preferred and thickness is only raised when nothing thinner can reach the
	target, so a sparse tier gets single-pixel walls on a wide lattice and only the densest
	tiers trade corridor width for heavier masonry.
	"""
	for thickness in range(1, 8):
		best = None
		# ceiling falls as the cell grows, so the widest lattice that still fits is the last
		# one before the scan breaks
		for cell in range(thickness + MAZE_MIN_CORRIDOR, 128):
			cols = max(2, mc.width // cell)
			rows = max(2, mc.height // cell)
			ceiling = _lattice_ceiling(mc, cell, thickness, cols, rows)
			if ceiling >= budget * MAZE_CEILING_MARGIN:
				best = cell
			else:
				break
		if best is not None:
			return best, thickness
	return 8, 1


@layout("maze")
def maze(mc: MapCanvas, rng: np.random.Generator, until_fill: float,
         style: list[tuple[tuple[int, int, int], str]], **_) -> dict:
	"""Grid maze: walls sit on the cell lattice, doors are lattice edges left open.

	Both knobs come from the area model rather than being fixed, because the lattice alone
	only spans one narrow density band. A cols x rows lattice of one-pixel walls covers about
	1/C of the map once the spanning tree's corridors are carved out, so the cell size is the
	widest one that still leaves room for the target, and the braid is then set to whatever
	share of the walls has to be removed to land on it:

		cell, thickness = _fit_lattice(...)  widest lattice whose walls can cover the budget
		braid = 1 - fill / ceiling          share of those walls knocked out for loops

	Braid then acts as the fine tuner between the lattice's coarse steps: the needed-wall pass
	paints (1 - braid) of the walls and the door-closing pass hands back the rest, so a 0.5%
	correction costs a different braid rather than a different cell size. Raising braid also
	loosens the maze, so a dense target yields a tight lattice and a sparse one a loose,
	looping one - which is also the right look for a sparse tier.

	The paint budget is the target plus the reserved disc, because `clear_protected` runs
	afterwards and would otherwise leave every dense map short by its own spawn clearing.
	"""
	head = max(until_fill, 0.005)
	# the disc is cleared after the layouts run, so a lattice has to cover this much to finish
	# on target: `head` plus the spawn clearing it is about to give back
	budget = min(0.98, head + float(mc.protected.mean()))

	cell, thickness = _fit_lattice(mc, budget)
	cols = max(2, mc.width // cell)
	rows = max(2, mc.height // cell)
	# Measure the ceiling instead of estimating it. A pure spanning tree walls every non-tree
	# edge, so carving once unbraided gives the exact area those walls would cover; anything
	# less than that has to come out of the braid. Estimating this as 1/cell over-promises by
	# enough to make the densest tier impossible - cell 3 walls at most 32.3% of the map, so
	# a 32% target has almost no braid available and any analytic slack silently lands short.
	probe, _, _ = _carve_grid(cols, rows, rng, 0.0)
	ceiling = cell * thickness * len(probe) / float(mc.width * mc.height)
	braid = float(np.clip(1.0 - head / max(ceiling, 1e-6), 0.0, 0.55))
	# centre the lattice so its walls never sit on the map edge
	ox = (mc.width - cols * cell) // 2
	oz = (mc.height - rows * cell) // 2
	vw, hw, braid_opened = _carve_grid(cols, rows, rng, braid)
	pick = _picker(rng, style)

	# only the braid-opened edges may be closed again. the tree edges carved during the DFS are
	# what keep the corridors one connected network, so closing those seals off whole regions.
	doors = sorted(braid_opened)
	rng.shuffle(doors)

	# the disc is cleared after the layouts run, so paint past the target by its area
	budget = min(0.98, until_fill + float(mc.protected.mean()))

	def wall_rect(axis: str, i: int, j: int) -> tuple[int, int, int, int]:
		if axis == "v":
			x, z = ox + i * cell, oz + j * cell
			return x, z, x + thickness, z + cell
		x, z = ox + i * cell, oz + j * cell
		return x, z, x + cell, z + thickness

	# one color per wall segment, so a run of wall reads as a single material
	segment_colors: dict[tuple, tuple[int, int, int]] = {}

	def paint_edge(axis: str, i: int, j: int, color):
		x0, z0, x1, z1 = wall_rect(axis, i, j)
		if not mc.protected[min(z0, mc.height - 1), min(x0, mc.width - 1)]:
			mc.rect(x0, z0, x1, z1, color)

	# Paint the walls this maze needs only until the budget is spent, then spend whatever is
	# left on re-closing doors. Painting the whole lattice first overshot the band whenever
	# earlier layouts had already used most of it.
	needed = ([("v", i, j) for i, j in vw] + [("h", i, j) for i, j in hw])
	rng.shuffle(needed)
	painted = 0
	for axis, i, j in needed:
		if mc.fill_to(budget):
			break
		paint_edge(axis, i, j, segment_colors.setdefault((axis, i, j), pick()[0]))
		painted += 1

	opened = 0
	for axis, i, j in doors:
		if mc.fill_to(budget):
			break
		paint_edge(axis, i, j, segment_colors.setdefault((axis, i, j), pick()[0]))
		opened += 1

	return dict(cell=cell, cols=cols, rows=rows, thickness=thickness,
	            braid=round(braid, 3), budget=round(budget, 4),
	            walls_needed=len(needed), walls_painted=painted,
	            doors_reclosed=opened, fill=round(mc.fill_fraction(), 4))


# ------------------------------------------------------------------
# columns: scattered pillars, reads as outdoors or a large indoor hall
# ------------------------------------------------------------------

@layout("columns")
def columns(mc: MapCanvas, rng: np.random.Generator, until_fill: float,
            style: list[tuple[tuple[int, int, int], str]], spacing: int | None = None,
            clumped: float = 0.35, **_) -> dict:
	"""Single-pixel (or short run) pillars scattered over open ground.

	Spacing is derived from the budget rather than fixed: `sparse` leaves wide gaps so the
	columns read individually, `dense` closes them into a colonnade. A fraction of columns
	are dropped next to an existing one so sparse maps clump into groves instead of
	salt-and-pepper noise.
	"""
	total = mc.width * mc.height
	needed = max(1, int(round(until_fill * total)) - int(mc.fill_fraction() * total))
	if spacing is None:
		# a column with spacing s reserves a (2s+1)^2 footprint, so the widest spacing whose
		# footprint still fits `needed` cells is floor((sqrt(total/needed) - 1) / 2). rounding
		# instead of flooring picks a spacing that runs the field dry before the target.
		spacing = int(np.clip(np.floor((np.sqrt(total / needed) - 1.0) / 2.0), 0, 5))

	allowed = mc.paintable.copy()
	placed = 0
	anchors: list[tuple[int, int]] = []
	attempts = 0
	max_attempts = needed * 6 + 64
	pick = _picker(rng, style)
	while placed < needed and attempts < max_attempts:
		attempts += 1
		cell = _claim(rng, allowed, anchors, spacing, clumped)
		if cell is None:
			break
		x, z = cell
		rgb, _obj = pick()
		# short runs read as a line of columns rather than a grid of dots
		run = 1 + int(rng.integers(3)) if rng.random() < 0.3 else 1
		horizontal = rng.random() < 0.5
		cells = 0
		for k in range(run):
			px, pz = (x + k, z) if horizontal else (x, z + k)
			if 0 <= px < mc.width and 0 <= pz < mc.height and not mc.protected[pz, px]:
				mc.paint(px, pz, rgb)
				_open_ring(allowed, px, pz, spacing)
				cells += 1
		if cells:
			anchors.append((x, z))
			placed += cells

	return dict(spacing=spacing, placed=placed, fill=round(mc.fill_fraction(), 4))


# ------------------------------------------------------------------
# rubble: loose blocks, reads as forest or a cluttered yard
# ------------------------------------------------------------------

@layout("rubble")
def rubble(mc: MapCanvas, rng: np.random.Generator, until_fill: float,
           style: list[tuple[tuple[int, int, int], str]], clump: float = 0.45,
           max_block: int = 3, **_) -> dict:
	"""Blocks of 1..max_block square dropped on open ground.

	Placed in bursts around an anchor so the result gathers into thickets at low density and
	fills in at high density.
	"""
	allowed = mc.paintable.copy()
	total = mc.width * mc.height
	needed = max(1, int(round(until_fill * total)) - int(mc.fill_fraction() * total))
	placed = 0
	anchors: list[tuple[int, int]] = []
	attempts = 0
	max_attempts = needed * 6 + 64
	pick = _picker(rng, style)

	while placed < needed and attempts < max_attempts:
		attempts += 1
		cell = _claim(rng, allowed, anchors, max(3, int(clump * 12)), clump)
		if cell is None:
			break
		x, z = cell
		rgb, _obj = pick()
		side = 1 + int(rng.integers(max_block))
		cells = 0
		for dz in range(side):
			for dx in range(side):
				px, pz = x + dx, z + dz
				if 0 <= px < mc.width and 0 <= pz < mc.height and not mc.protected[pz, px]:
					mc.paint(px, pz, rgb)
					cells += 1
		if cells:
			anchors.append((x, z))
			# blocks are allowed to touch at higher densities - that is what turns a sparse
			# scatter into a thicket - so only the block's own cells leave the field
			_open_ring(allowed, x + side // 2, z + side // 2, 0, width=side, height=side)
			placed += cells

	return dict(placed=placed, fill=round(mc.fill_fraction(), 4))


def _top_up(mc: MapCanvas, rng: np.random.Generator, target: float,
            style: list[tuple[tuple[int, int, int], str]], max_block: int = 2) -> int:
	"""Scatter small blocks over open ground until the canvas reaches `target`.

	A layout stops early whenever its placement strategy runs out of room: `columns` needs a
	clear ring around every pillar and `rubble` needs a block-sized gap, so at the dense end
	both can run the field dry a percent or two below target. Left alone that shows up as a
	dense world quietly landing in the medium tier. This tops the shortfall up with the same
	small blocks the layout was placing, so the map reads as the same kind of clutter at a
	density the tier name actually promises.
	"""
	pick = _picker(rng, style)
	added = 0
	while mc.fill_fraction() < target:
		allowed = mc.paintable
		ys, xs = np.where(allowed)
		if len(xs) == 0:
			break
		# a random order over the free cells, advanced one block at a time, so the shortfall is
		# spread over the whole map instead of piling into whichever corner was scanned first
		i = int(rng.integers(len(xs)))
		x, z = int(xs[i]), int(ys[i])
		side = 1 + int(rng.integers(max_block + 1))
		rgb, _obj = pick()
		for dz in range(side):
			for dx in range(side):
				px, pz = x + dx, z + dz
				if 0 <= px < mc.width and 0 <= pz < mc.height:
					mc.paint(px, pz, rgb)
					added += 1
	return added


def ensure_reachable(mc: MapCanvas, min_reachable: float, lane: int = 5,
                     max_lanes: int = 12) -> tuple[float, int]:
	"""Carve entry lanes until enough open ground connects to the spawn point.

	A maze that paints a wall lattice around the origin leaves the robot in a sealed pocket,
	which keeping a spawn disc clear does not prevent. Each lane runs from the origin to the
	unreachable open cell nearest it; because that cell's whole region then touches the
	reachable set, one lane merges an entire region. validate.py then MEASURES the result
	rather than this function assuming it worked.
	"""
	reachable, lanes = 0.0, 0
	for _ in range(max_lanes):
		seen, reachable = reachable_from_origin(mc)
		if reachable >= min_reachable:
			break
		unreached = mc.empty & ~seen
		if not unreached.any():
			break
		zs, xs = np.where(unreached)
		# nearest unreachable cell to the origin: its region is what we want to join
		i = int(np.argmin(xs * xs + zs * zs))
		target_x, target_z = int(xs[i]), int(zs[i])
		# a lane wide enough to drive, drawn as parallel offsets of the same line
		for off in range(lane // 2 + 1):
			mc.line(0, 0, target_x + off, target_z, EMPTY_RGB)
			mc.line(off, 0, target_x, target_z + off, EMPTY_RGB)
		lanes += 1
	reachable = reachable_from_origin(mc)[1]
	return reachable, lanes


# ------------------------------------------------------------------
# composition
# ------------------------------------------------------------------

def generate_map(width: int, height: int, layouts: list[str], target_fill: float,
                 styles: list[list[tuple[str, str]]], seed: int | None = None,
                 spawn_radius: int = SPAWN_CLEAR_RADIUS,
                 min_reachable: float = MIN_REACHABLE) -> tuple[MapCanvas, list[dict]]:
	"""Run `layouts` over a fresh canvas until it reaches `target_fill`.

	`styles` is one list of (color_name, object_name) pairs per layout - a layout draws from
	its own list, so layers never share a color and every declared color ends up on the map.
	Returns the canvas plus a per-layout report for the manifest.
	"""
	unknown = [n for n in layouts if n not in LAYOUTS]
	if unknown:
		raise ValueError(f"unknown layout(s) {unknown} - known: {', '.join(LAYOUTS)}")
	if len(styles) != len(layouts):
		raise ValueError(f"need one style list per layout: {len(layouts)} layouts, "
						 f"{len(styles)} style lists")
	for name, style in zip(layouts, styles):
		if not style:
			raise ValueError(f"layout '{name}' got an empty style list")

	rng = np.random.default_rng(seed)
	mc = MapCanvas(width, height)
	mc.reserve_spawn(spawn_radius)

	report = []
	n = len(layouts)
	for i, name in enumerate(layouts):
		style = [(COLOR_RGB[c], o) for c, o in styles[i]]
		until = target_fill * (i + 1) / n
		stats = LAYOUTS[name](mc, rng, until, style)
		report.append(dict(layout=name,
		                   colors=[c for c, _ in styles[i]],
		                   objects=sorted({o for _, o in styles[i]}),
		                   until_fill=round(until, 4), **stats))

	# a layout must never have walled the spawn in
	mc.clear_protected()

	# any layout that ran out of room leaves the canvas under target, and giving the spawn disc
	# back takes a little more off. The shortfall is filled with the last layout's own blocks, so
	# the map reads as the same kind of clutter at a density the tier name actually promises.
	short = _top_up(mc, rng, target_fill, [(COLOR_RGB[c], o) for c, o in styles[-1]])
	if short:
		report.append(dict(layout="top_up", blocks=short,
		                   fill=round(mc.fill_fraction(), 4)))

	reachable, lanes = ensure_reachable(mc, min_reachable)
	report.append(dict(final_fill=round(mc.fill_fraction(), 4),
	                   target_fill=round(target_fill, 4),
	                   open_at_origin=bool(mc.is_empty(0, 0)),
	                   reachable=round(reachable, 3), entry_lanes=lanes))
	return mc, report


def plan_layouts(rng: np.random.Generator, allowed: list[str],
                 max_layouts: int = 3) -> list[str]:
	"""Pick 1..max_layouts distinct layouts to layer for one world."""
	pool = [n for n in allowed if n in LAYOUTS]
	if not pool:
		raise ValueError(f"none of {allowed} are known layouts - known: {', '.join(LAYOUTS)}")
	k = 1 + int(rng.integers(min(max_layouts, len(pool))))
	return [pool[int(i)] for i in rng.choice(len(pool), size=k, replace=False)]