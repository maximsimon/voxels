# generate_all.py - CLI entry point for world generation
# usage: python -m scripts.worldgen.generate_all --dataset myds --count 12
#        python -m scripts.worldgen.generate_all --list-layouts

import argparse
from .atlas import atlas_sets, build_atlas_sets
from .dataset import GROUND_POOL, SKY_POOL, generate_dataset
from .layouts import DENSITY_TIERS, LAYOUTS, TIER_ORDER


def _csv(value: str) -> list[str]:
	return [v.strip() for v in value.split(",") if v.strip()]


def main():
	p = argparse.ArgumentParser(description="generate world dataset for Voxel World RL")
	p.add_argument("--dataset", help="dataset name (output folder + registry name)")
	p.add_argument("--count", type=int, default=12, help="number of worlds")
	p.add_argument("--width", type=int, default=256, help="map width in pixels (multiple of 16)")
	p.add_argument("--height", type=int, default=256, help="map height in pixels (multiple of 16)")
	p.add_argument("--seed", type=int, default=0, help="base RNG seed; world i uses seed+i")
	p.add_argument("--layouts", type=_csv, default=sorted(LAYOUTS),
	               help=f"layouts to draw from (default: all). known: {', '.join(LAYOUTS)}")
	p.add_argument("--layouts-per-world", type=int, default=3,
	               help="max layouts layered into one world")
	p.add_argument("--density", type=_csv, default=TIER_ORDER,
	               help=f"density tiers, interleaved evenly (default: all). "
	                    f"known: {', '.join(DENSITY_TIERS)}")
	p.add_argument("--palette-min", type=int, default=4, help="fewest colors per world")
	p.add_argument("--palette-max", type=int, default=7, help="most colors per world")
	p.add_argument("--atlas-cols", type=int, default=4, help="atlas columns")
	p.add_argument("--atlas-rows", type=int, default=4, help="atlas rows")
	p.add_argument("--atlas-set", help=f"force one atlas set. known: {', '.join(atlas_sets())}")
	p.add_argument("--ground-pool", type=_csv, default=GROUND_POOL,
	               help=f"ground textures to sample from. default: {','.join(GROUND_POOL)}")
	p.add_argument("--sky-pool", type=_csv, default=SKY_POOL,
	               help=f"sky textures to sample from. default: {','.join(SKY_POOL)}")
	p.add_argument("--spawn-radius", type=int, default=10,
	               help="px around the origin kept clear for the trainer's spawn poses")
	p.add_argument("--no-validate", action="store_true",
	               help="skip validation (not recommended - it is what catches unusable maps)")
	p.add_argument("--atlas-only", action="store_true", help="only build the atlas sets")
	p.add_argument("--list-layouts", action="store_true", help="print the known layouts and exit")
	p.add_argument("--list-atlases", action="store_true", help="print the known atlas sets and exit")
	args = p.parse_args()

	if args.list_layouts:
		print(f"layouts: {', '.join(LAYOUTS)}")
		print(f"tiers:   {', '.join(f'{t} {DENSITY_TIERS[t][0]:.0%}-{DENSITY_TIERS[t][1]:.0%}' for t in TIER_ORDER)}")
		return
	if args.list_atlases:
		print(f"atlas sets: {', '.join(atlas_sets())}")
		return

	if args.atlas_only:
		for name, path in build_atlas_sets(args.atlas_cols, args.atlas_rows).items():
			print(f"  {name}: {path}")
		return

	if not args.dataset:
		p.error("--dataset is required")

	registry = generate_dataset(
		name=args.dataset,
		count=args.count,
		width=args.width,
		height=args.height,
		base_seed=args.seed,
		layouts=args.layouts,
		density=args.density,
		palette_min=args.palette_min,
		palette_max=args.palette_max,
		max_layouts=args.layouts_per_world,
		atlas_cols=args.atlas_cols,
		atlas_rows=args.atlas_rows,
		ground_pool=args.ground_pool,
		sky_pool=args.sky_pool,
		atlas_set=args.atlas_set,
		spawn_radius=args.spawn_radius,
		validate=not args.no_validate,
	)

	rel = registry.relative_to(registry.parents[3])
	print(f"\npoint the RL config at it:")
	print(f"  world_folder = core_voxels/resources/generated/{args.dataset}")


if __name__ == "__main__":
	main()