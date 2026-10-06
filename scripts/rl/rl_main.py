#!/usr/bin/env python3
"""
RL MAIN - the entry point of the reinforcement learning setup. It starts the VecEnv from scripts/bare_bones, hands the chosen policy, reward and algorithm to the trainer and runs the workflow to completion.
Run it from the repository root with:

	PYTHONPATH=build python3 scripts/rl/rl_main.py

This file holds only the wiring, the three pluggable pieces. Every number lives in rl_config.py, and RLConfig() picks them all up, so nothing here can shadow them.
"""
from __future__ import annotations

import os
import sys

_HERE = os.path.dirname(os.path.abspath(__file__))
_ROOT = os.path.dirname(os.path.dirname(_HERE))
sys.path.insert(0, os.path.join(_ROOT, "build"))				# voxel_sim.so, produced by cmake into build/
sys.path.insert(0, os.path.join(_ROOT, "scripts", "vector_env"))	# vec_env.py and its wrapper window

from vec_env import VecEnv	# noqa: E402
from algorithm import PlaceholderAlgorithm	# noqa: E402
from policy import ConstantPolicy, RandomPolicy	# noqa: E402
from reward import blocked_velocity_reward, forward_velocity_reward	# noqa: E402
from rl_config import RLConfig	# noqa: E402
from trainer import Trainer	# noqa: E402

# WIRING - the three pluggable pieces of the setup, chosen here and nowhere else. Replace any of them with your own and the workflow runs unchanged.
# a policy is a class taking (num_envs, action_dim, image_shape, state_dim, seed=...) with an act(images, states, step) method; ConstantPolicy and RandomPolicy are the two placeholders
POLICY = RandomPolicy
# a reward is a plain function taking (obs, actions) and returning one float per environment; blocked_velocity_reward is the other placeholder
REWARD = forward_velocity_reward
# an algorithm is a class taking (image_shape, state_dim, action_dim, num_envs, seed=..., batch_size=...) with an update(buffer) method
ALGORITHM = PlaceholderAlgorithm


# RUN CONFIGURATION - every number this run uses comes from rl_config.py, so there is exactly one place to edit and nothing here can shadow it.
# to try a variant without touching the defaults, override only what changes: RLConfig(train=TrainConfig(iterations=200))


def main():
	config = RLConfig()
	print(f"rl: {POLICY.__name__} + {REWARD.__name__} + {ALGORITHM.__name__}")
	worlds = f"folder {config.env.world_folder}" if config.env.world_folder else config.env.world
	print(f"env: {config.env.num_envs} instances, worlds from {worlds}, camera {config.env.camera_width}x{config.env.camera_height}, "
		  f"{config.env.lidar_rays} lidar rays, mosaic={config.env.show_window}")

	with VecEnv(config.env.num_envs, config=config.env.to_sim_config(),
				start_method=config.env.start_method, show_mosaic=config.env.show_window) as env:
		trainer = Trainer(env, POLICY, REWARD, ALGORITHM, config)
		result = trainer.run()

	print("done:", {k: result[k] for k in ("iteration", "reward_mean", "return_mean", "steps_per_second")})


if __name__ == "__main__":
	main()
