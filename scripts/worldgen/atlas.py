# atlas.py - tile grids for world appearance.
#
# the C++ picks a tile by index and nothing else, so the atlas is just a palette of
# sub-rectangles: index -> (label, source image). Each named set below is a different
# arrangement of the same source photos, which is what makes two worlds that share an atlas
# file size look different.
#
# capacity matters: a world needs one distinct tile per color, and the generated sets are 4x4
# (16 slots) so palettes of up to 8 colors always fit. the hand-made 2x2 atlases in
# resources/textures (atlas.png, city_atlas.png, texture_atlas.png, desert_texture_atlas.png)
# only offer 4 slots and are therefore usable only by a <=4-color world.

from pathlib import Path
from PIL import Image

_TREE = "tree.jpg"
_BARK = "bark.jpg"
_BUILDING = "building.jpg"
_BUSH = "bush.png"
_FENCE = "fence.jpg"
_PUDDLE = "puddle.png"
_GRID = "the_grid_tile.png"
_ASPHALT = "asphalt.png"

_TEX_DIR = Path(__file__).resolve().parents[2] / "core_voxels" / "resources" / "textures"
TILE_SIZE = 256

# named 16-slot grids. every entry is a source photo; the label is only for debugging, the
# registry addresses tiles by index.
ATLAS_SETS: dict[str, tuple[str, ...]] = {
	"park": (_TREE, _BUSH, _BARK, _GRID, _PUDDLE, _BUSH, _TREE, _FENCE,
			 _BARK, _GRID, _TREE, _PUDDLE, _BUSH, _FENCE, _BARK, _GRID),
	"stone": (_BARK, _ASPHALT, _GRID, _BUILDING, _BARK, _ASPHALT, _GRID, _FENCE,
			  _ASPHALT, _BUILDING, _BARK, _GRID, _ASPHALT, _FENCE, _BARK, _GRID),
	"wild": (_TREE, _BUSH, _PUDDLE, _TREE, _BARK, _GRID, _BUSH, _TREE,
			 _PUDDLE, _FENCE, _TREE, _GRID, _BUSH, _BARK, _TREE, _PUDDLE),
}
DEFAULT_SET = "park"

# kept for callers that want the index -> (label, path) form
ATLAS_TILES = {i: (Path(p).stem, f"../../core_voxels/resources/textures/{p}")
			   for i, p in enumerate(ATLAS_SETS[DEFAULT_SET])}


def atlas_sets() -> list[str]:
	return sorted(ATLAS_SETS)


# which source photo suits each object. The registry can address a tile per object
# (object_texture.<name>=i) as well as per color, and that is the honest mapping here: a color
# is only a hue for the C++ matcher, so a red wall and a green wall of the same object would
# otherwise differ in color but share one photo. Keyed by source file, so a set that lacks a
# photo falls back to whatever it does have.
OBJECT_SOURCES: dict[str, tuple[str, ...]] = {
	"tree": (_TREE, _BARK),
	"bush": (_BUSH, _TREE),
	"rock": (_ASPHALT, _GRID),
	"sand": (_ASPHALT, _GRID),
	"pillar": (_BARK, _ASPHALT),
	"fence": (_FENCE, _GRID),
	"puddle": (_PUDDLE, _ASPHALT),
	"crate": (_BARK, _BUILDING),
	"brick": (_BUILDING, _GRID),
	"building": (_BUILDING, _GRID),
}


def object_tiles(name: str = DEFAULT_SET) -> dict[str, int]:
	"""Tile index per object for a named atlas set.

	Every object gets a distinct slot where the set allows it, so two objects of the same
	shape do not end up wearing the same photo. Only reached when a set is short of sources;
	the generated 4x4 sets have 16 slots and there are 10 objects, so this is the normal case.
	"""
	sources = ATLAS_SETS[name]
	out: dict[str, int] = {}
	taken: set[int] = set()
	for obj, prefs in OBJECT_SOURCES.items():
		for pref in prefs:
			if pref in sources and sources.index(pref) not in taken:
				out[obj] = sources.index(pref)
				taken.add(sources.index(pref))
				break
	# anything still unplaced takes the lowest free slot
	for obj in OBJECT_SOURCES:
		if obj in out:
			continue
		for i in range(len(sources)):
			if i not in taken:
				out[obj] = i
				taken.add(i)
				break
	return out


def atlas_path(name: str = DEFAULT_SET, ext: str = "png") -> Path:
	"""Where build_atlas_sets writes (and the registry reads) a named atlas."""
	return _TEX_DIR / f"atlas_{name}.{ext}"


def tile_uv(index: int, cols: int = 4, rows: int = 4):
	"""Return (u_min, v_min, u_max, v_max) in [0,1] for the tile at `index`."""
	z = index // cols
	x = index % cols
	u_min = x / cols
	u_max = (x + 1) / cols
	v_min = z / rows
	v_max = (z + 1) / rows
	return u_min, v_min, u_max, v_max


def _square_tile(filename: str, size: int = TILE_SIZE) -> Image.Image:
	"""Center-crop a source photo square and resize it to one atlas cell."""
	img = Image.open(_TEX_DIR / filename).convert("RGBA")
	w, h = img.size
	side = min(w, h)
	img = img.crop(((w - side) // 2, (h - side) // 2,
	                (w - side) // 2 + side, (h - side) // 2 + side))
	return img.resize((size, size), Image.LANCZOS)


def build_atlas(cols: int = 4, rows: int = 4, out: str | Path | None = None,
                sources: tuple[str, ...] | None = None) -> dict[int, str]:
	"""Lay a tile grid out as one PNG. Returns {index: source label}.

	`sources` defaults to the DEFAULT_SET. Slots past the end of the manifest repeat its last
	entry, so a smaller manifest still fills a larger grid.
	"""
	sources = sources or ATLAS_SETS[DEFAULT_SET]
	out = Path(out) if out is not None else atlas_path(DEFAULT_SET)
	out.parent.mkdir(parents=True, exist_ok=True)

	capacity = cols * rows
	atlas = Image.new("RGBA", (TILE_SIZE * cols, TILE_SIZE * rows), (0, 0, 0, 0))
	labels: dict[int, str] = {}
	for idx in range(capacity):
		src = sources[min(idx, len(sources) - 1)]
		atlas.paste(_square_tile(src), ((idx % cols) * TILE_SIZE, (idx // cols) * TILE_SIZE))
		labels[idx] = Path(src).stem

	atlas.save(str(out))
	print(f"atlas: saved {out} ({cols}x{rows}, {TILE_SIZE}px/tile)")
	return labels


def build_atlas_sets(cols: int = 4, rows: int = 4,
                     sets: list[str] | None = None) -> dict[str, Path]:
	"""Build every named atlas set that is not already on disk. Returns {set: path}."""
	paths = {}
	for name in (sets or atlas_sets()):
		path = atlas_path(name)
		if not path.exists():
			build_atlas(cols, rows, path, sources=ATLAS_SETS[name])
		paths[name] = path
	return paths


def slice_atlas(path: str | Path, cols: int = 4, rows: int = 4) -> list[Image.Image]:
	"""Slice an existing atlas PNG into individual tile images (for inspection/debug)."""
	img = Image.open(path)
	w, h = img.size
	tw, th = w // cols, h // rows
	tiles = []
	for z in range(rows):
		for x in range(cols):
			tiles.append(img.crop((x * tw, z * th, (x + 1) * tw, (z + 1) * th)))
	return tiles