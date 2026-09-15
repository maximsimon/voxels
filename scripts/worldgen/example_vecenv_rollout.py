# example_vecenv_rollout.py - demonstrate dataset worlds in a headless VecEnv loop
# usage:
#   1. generate a dataset:  python -m scripts.worldgen.generate_all --dataset myds --count 5
#   2. point at it:        edit worlds.config to add registry=.../myds.registry
#   3. run:                PYTHONPATH=build python scripts/worldgen/example_vecenv_rollout.py [spawn_x] [spawn_z]

import sys
sys.path.insert(0, "build")

import argparse
from vec_env import VecEnv
import voxel_sim as v
import numpy as np

def main():
	#spawn_x = float(sys.argv[1]) if len(sys.argv) > 1 else 32.0
	#spawn_z = float(sys.argv[2]) if len(sys.argv) > 2 else 32.0
	ap = argparse.ArgumentParser()
	ap.add_argument("--show", action="store_true", help="open wrapper window (h=hide, s=show)")
	args = ap.parse_args()

	print("WORLDS:", v.WORLDS)
	if len(v.WORLDS) == 0:
		print("no worlds registered - run generate_all first and point worlds.config at the dataset registry")
		return

	actions = np.zeros((len(v.WORLDS), 6), dtype=np.float32)
	actions[:, 0] = 1.5
	actions[:, 4] = 1.2
	
	with VecEnv(len(v.WORLDS), show_window=args.show) as env:
		env.reset(v.WORLDS)

		for i in range(1000):
			env.step(actions)
			print("running")
"""
	v.Configure(target_fps=0, render_gui=False)
	obs = v.Init()
	print("Init OK, running:", obs["running"])

	# reset into each world for a few steps
	for world_name in v.WORLDS:
		obs = v.Reset(world_name, (spawn_x, spawn_z, 0.0))
		print(f"\n  {world_name}: pos={obs['position']}, cam_shape={obs['camera_front'].shape}")
		for step in range(5):
			action = np.array([1.0, 0.0, 0.0, 0.0, 0.0, 0.0], dtype=np.float32)
			obs = v.Step(action)
			if not obs["running"]:
				break
		print(f"    after 5 steps: pos={obs['position']}")

	v.Close()
	print("\nall done")
"""
if __name__ == "__main__":
	main()
