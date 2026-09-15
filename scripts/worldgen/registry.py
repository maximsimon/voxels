# registry.py - write worlds.registry-compatible text files
# world_block builds the raw text block; write/append_registry writes files.

from pathlib import Path


def world_block(name: str, *, map_path: str, atlas_path: str, ground_path: str, sky_path: str,
                atlas_cols: int = 4, atlas_rows: int = 4,
                colors: list[str] | None = None,
                tiles: dict[str, int] | None = None,
                objects: dict[str, str] | None = None,
                object_textures: dict[str, int] | None = None) -> str:
	"""Build a single world= block for a worlds.registry file.

	all paths are relative to the voxel world root (core_voxels/resources/...).
	colors is a list of palette names (red, green, ...).
	tiles maps color_name -> atlas tile index.
	objects maps color_name -> object library name (tree, brick, ...).
	object_textures maps object_name -> explicit atlas tile index (optional).
	"""
	lines = [f"world={name}"]
	lines.append(f"map={map_path}")
	lines.append(f"atlas={atlas_path}")
	lines.append(f"ground={ground_path}")
	lines.append(f"sky={sky_path}")
	lines.append(f"atlas_cols={atlas_cols}")
	lines.append(f"atlas_rows={atlas_rows}")
	if colors:
		lines.append(f"colors={','.join(colors)}")
	if tiles:
		for color_name, idx in tiles.items():
			lines.append(f"tile.{color_name}={idx}")
	if objects:
		for color_name, obj_name in objects.items():
			lines.append(f"object.{color_name}={obj_name}")
	if object_textures:
		for obj_name, idx in object_textures.items():
			lines.append(f"object_texture.{obj_name}={idx}")
	lines.append("")
	return "\n".join(lines)


def write_registry(path: str | Path, blocks: list[str]):
	"""Write a registry file from a list of world block strings."""
	path = Path(path)
	path.parent.mkdir(parents=True, exist_ok=True)
	with open(path, "w") as f:
		for b in blocks:
			f.write(b)
			f.write("\n")
	print(f"registry: wrote {path} ({len(blocks)} world(s))")


def append_registry(path: str | Path, block: str):
	"""Append one world block to an existing (or new) registry file."""
	path = Path(path)
	path.parent.mkdir(parents=True, exist_ok=True)
	with open(path, "a") as f:
		f.write(block)
		f.write("\n")
