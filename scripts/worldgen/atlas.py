# 16-tile shared atlas builder
# Hand-maintained manifest maps tile index -> (name, image_path).
# Repeat/reuse textures to fill 4x4 grid; swap in real images later.
# build_atlas() produces shared_atlas.png at the target grid size.
# tile_z = index // cols, tile_x = index % cols matches fetchTextureCoords C++ grid math.

from pathlib import Path
from PIL import Image

# atlas tile manifest: index -> (name, path relative to this file's directory)
ATLAS_TILES = {
	0:  ("tree",          "../../core_voxels/resources/textures/tree.jpg"),
	1:  ("brick",         "../../core_voxels/resources/textures/bark.jpg"),
	2:  ("building",      "../../core_voxels/resources/textures/building.jpg"),
	3:  ("bush",          "../../core_voxels/resources/textures/bush.png"),
	4:  ("fence",         "../../core_voxels/resources/textures/fence.jpg"),
	5:  ("puddle",        "../../core_voxels/resources/textures/puddle.png"),
	6:  ("sand",          "../../core_voxels/resources/textures/bush.png"),
	7:  ("rock",          "../../core_voxels/resources/textures/bark.jpg"),
	8:  ("crate",         "../../core_voxels/resources/textures/the_grid_tile.png"),
	9:  ("pillar",        "../../core_voxels/resources/textures/the_grid_tile.png"),
	10: ("flower",        "../../core_voxels/resources/textures/the_grid_tile.png"),
	11: ("mushroom",      "../../core_voxels/resources/textures/the_grid_tile.png"),
	12: ("sign",          "../../core_voxels/resources/textures/the_grid_tile.png"),
	13: ("post",          "../../core_voxels/resources/textures/the_grid_tile.png"),
	14: ("barrel",        "../../core_voxels/resources/textures/the_grid_tile.png"),
	15: ("bucket",        "../../core_voxels/resources/textures/the_grid_tile.png"),
}

_DATA_DIR = Path(__file__).parent


def _resolve(path: str) -> Path:
	return (_DATA_DIR / path).resolve()


def tile_uv(index: int, cols: int = 4, rows: int = 4):
	"""Return (u_min, v_min, u_max, v_max) in [0,1] for the tile at `index`."""
	z = index // cols
	x = index % cols
	u_min = x / cols
	u_max = (x + 1) / cols
	v_min = z / rows
	v_max = (z + 1) / rows
	return u_min, v_min, u_max, v_max


def build_atlas(cols: int = 4, rows: int = 4, out: str | Path | None = None) -> dict[int, str]:
	"""Build a `cols`x`rows` tile grid from the manifest, save as out, return {index: name}.

	Tiles are cropped to their center square then resized to tile_size.
	Indices beyond len(ATLAS_TILES) repeat the last tile to fill the grid.
	out defaults to core_voxels/resources/textures/shared_atlas.png relative to the repo root.
	"""
	if out is None:
		out = _DATA_DIR / "../../resources/textures/shared_atlas.png"
	out = Path(out)
	out.parent.mkdir(parents=True, exist_ok=True)

	capacity = cols * rows
	# square-crop + resize every tile to tile_size
	# tile_size is chosen so the full atlas is reasonably large; each tile gets 256px
	TILE_SIZE = 256

	last_img = None
	for idx in range(capacity):
		key = min(idx, max(ATLAS_TILES.keys()))
		name, rel = ATLAS_TILES[key]
		img = Image.open(_resolve(rel)).convert("RGBA")
		last_img = img
		# center crop to square
		w, h = img.size
		side = min(w, h)
		left = (w - side) // 2
		top = (h - side) // 2
		img = img.crop((left, top, left + side, top + side))
		img = img.resize((TILE_SIZE, TILE_SIZE), Image.LANCZOS)
		# paste into the grid
		z = idx // cols
		x = idx % cols
		if idx == 0:
			atlas = Image.new("RGBA", (TILE_SIZE * cols, TILE_SIZE * rows), (0, 0, 0, 0))
		atlas.paste(img, (x * TILE_SIZE, z * TILE_SIZE))

	atlas.save(str(out))
	print(f"atlas: saved {out} ({cols}x{rows}, {TILE_SIZE}px/tile)")

	# return the name map for registry writers
	return {idx: ATLAS_TILES[min(idx, max(ATLAS_TILES.keys()))][0] for idx in range(capacity)}


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
