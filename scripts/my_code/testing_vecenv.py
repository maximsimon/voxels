#!/usr/bin/env python3

import argparse

import numpy as np

from vec_env import VecEnv
import voxel_sim

def main():
	ap = argparse.ArgumentParser()
	ap.add_argument("--show", action="store_true",
					help="open wrapper window (h=hide, s=show)")
	ap.add_argument("--steps", type=int, default=200, help="steps to run (default: 200)")
	args = ap.parse_args()

	actions = np.zeros((18, 6), dtype=np.float32)
	actions[:, 0] = 1.5
	actions[:, 4] = 1.2

	total_rew = 0

	with VecEnv(18, show_window=args.show) as env:
		obs = env.reset("ConferenceWorld", None)
		for _ in range(args.steps):
			print("running")
			obs, rew, ter, trunc, info = env.step(actions)
			total_rew += rew

	print("final obs:", obs)
	print("final rew:", rew)
	print("ter:", ter)
	print("total_rew:", total_rew)

if __name__ == "__main__":
	main()
