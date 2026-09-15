# dataset.py - generate a complete dataset: atlas + map variants + registry file
# output goes to core_voxels/resources/generated/<dataset_name>/
# uses the shared atlas at core_voxels/resources/textures/shared_atlas.png

from pathlib import Path
from .atlas import build_atlas
from .mapgen import generate_variant
from .registry import world_block, write_registry

_REPO_ROOT = Path(__file__).resolve().parents[2]
_TEX_DIR = _REPO_ROOT / "core_voxels" / "resources" / "textures"
_GEN_DIR = _REPO_ROOT / "core_voxels" / "resources" / "generated"

# default world block settings
_DEFAULTS = dict(
	atlas_cols=4,
	atlas_rows=4,
	colors=["red", "green", "blue", "pink"],
	tiles={"red": 0, "green": 1, "blue": 3, "pink": 2},
	objects={"red": "tree", "green": "brick", "blue": "bush", "pink": "building"},
)


def generate_dataset(name: str, count: int = 10,
                     width: int = 256, height: int = 256,
                     rooms: int = 4, base_seed: int = 0,
                     atlas_cols: int = 4, atlas_rows: int = 4,
                     ground: str = "grass.png", sky: str = "stars.png") -> Path:
	"""Generate `count` map variants + one <name>.registry file.

	returns the path to the registry file.
	"""
	out_dir = _GEN_DIR / name
	out_dir.mkdir(parents=True, exist_ok=True)

	# build the shared atlas if it doesn't exist yet (or force rebuild)
	atlas_out = _TEX_DIR / "shared_atlas.png"
	build_atlas(cols=atlas_cols, rows=atlas_rows, out=atlas_out)

	blocks = []
	for i in range(count):
		seed = base_seed + i
		world_name = f"{name}_map{i:03d}"
		map_path = f"core_voxels/resources/generated/{name}/{world_name}.png"

		mc = generate_variant(width, height, rooms=rooms, seed=seed)
		mc.save_png(out_dir / f"{world_name}.png")

		blocks.append(world_block(
			name=world_name,
			map_path=map_path,
			atlas_path="core_voxels/resources/textures/shared_atlas.png",
			ground_path=f"core_voxels/resources/textures/{ground}",
			sky_path=f"core_voxels/resources/textures/{sky}",
			atlas_cols=atlas_cols,
			atlas_rows=atlas_rows,
			colors=_DEFAULTS["colors"],
			tiles=_DEFAULTS["tiles"],
			objects=_DEFAULTS["objects"],
		))

	registry_path = out_dir / f"{name}.registry"
	write_registry(registry_path, blocks)
	return registry_path
