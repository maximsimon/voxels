# generate_all.py - CLI entry point for world generation
# usage: python -m scripts.worldgen.generate_all --dataset myds --count 50

import argparse
from pathlib import Path
from .dataset import generate_dataset
from .atlas import build_atlas

def main():
	p = argparse.ArgumentParser(description="generate world dataset for Voxel World RL")
	p.add_argument("--dataset", required=True, help="dataset name (output folder + registry name)")
	p.add_argument("--count", type=int, default=10, help="number of map variants")
	p.add_argument("--width", type=int, default=256, help="map width in pixels (multiple of 16)")
	p.add_argument("--height", type=int, default=256, help="map height in pixels (multiple of 16)")
	p.add_argument("--rooms", type=int, default=4, help="rooms per map")
	p.add_argument("--seed", type=int, default=0, help="base RNG seed")
	p.add_argument("--atlas-cols", type=int, default=4, help="shared atlas columns")
	p.add_argument("--atlas-rows", type=int, default=4, help="shared atlas rows")
	p.add_argument("--ground", default="grass.png", help="ground texture filename")
	p.add_argument("--sky", default="stars.png", help="sky texture filename")
	p.add_argument("--atlas-only", action="store_true", help="only build the atlas, skip map generation")
	args = p.parse_args()

	if args.atlas_only:
		build_atlas(cols=args.atlas_cols, rows=args.atlas_rows)
		return

	registry = generate_dataset(
		name=args.dataset,
		count=args.count,
		width=args.width,
		height=args.height,
		rooms=args.rooms,
		base_seed=args.seed,
		atlas_cols=args.atlas_cols,
		atlas_rows=args.atlas_rows,
		ground=args.ground,
		sky=args.sky,
	)
	print(f"\ndataset '{args.dataset}' ready: {registry}")
	print(f"add to worlds.config:")
	print(f"  registry=core_voxels/resources/generated/{args.dataset}/{args.dataset}.registry")
	print(f"  world={args.dataset}_map000")


if __name__ == "__main__":
	main()
