#!/usr/bin/env python3
"""
RL CONFIG - every knob the reinforcement learning workflow reads lives here, so the trainer, the policy and the reward stay free of magic numbers.
Two dataclasses and a bundle: EnvConfig goes to the simulator through VecEnv, TrainConfig drives the workflow loop, and RLConfig is what the trainer takes.
Deliberately no policy or reward names here: which behaviour runs is decided by the wiring block at the top of rl_main, where real classes and functions get handed to the trainer.
"""
from __future__ import annotations

from dataclasses import dataclass, field

# the observation is split into two branches, because they want opposite things: pixels keep their spatial layout and their uint8 dtype, everything else gets flattened into one small vector.
# pixel branch, handed to the policy as uint8 [num_envs, H, W, C] and never cast, so a rollout costs ~8 MB instead of the ~32 MB a float32 copy of the same frames would
IMAGE_KEYS = ("camera_front",)
# state branch, handed to the policy as float32 [num_envs, sum of the listed widths]
STATE_KEYS = ("lidar_scan", "angular_vel", "linear_vel")


# simulator-side settings, forwarded to VecEnv as per-env configuration overrides
# show_window is deliberately NOT forwarded: in an RL run it means "open the one mosaic window", and VecEnv names that switch show_mosaic. The per-sim raylib windows stay hidden, so six envs never open six "You're in voxels now" windows.
@dataclass
class EnvConfig:
	num_envs: int = 4 
	world: str = "ConferenceWorld"
	# folder of generated worlds to draw episodes from (any folder holding exactly one *.registry). every episode starts in a world picked at random from it, different from the one that just ended, so the agent cannot memorise a single map. loading it REPLACES the classic world table, which makes `world` above unused - it only applies while this is empty.
	world_folder: str = "core_voxels/resources/generated/test2/"
	spawn_radius: float = 6.0
	start_method: str = "fork"
	show_window: bool = True
	camera_width: int = 128
	camera_height: int = 80
	lidar_rays: int = 180
	max_linear_speed: float = 3.0
	max_angular_speed: float = 1.5
	fixed_dt: float = 0.02

	# translate into the keyword-argument dict voxel_sim.Configure() expects
	def to_sim_config(self) -> dict:
		return dict(
			verbose=False,
			target_fps=0,
			render_gui=False,
			keyboard_enabled=False,
			quit_key_enabled=False,
			camera_width=self.camera_width,
			camera_height=self.camera_height,
			lidar_rays=self.lidar_rays,
			fast_collisions=True,
			max_linear_speed=self.max_linear_speed,
			max_angular_speed=self.max_angular_speed,
			fixed_dt=self.fixed_dt,
		)


# workflow-side settings: how much data to collect, how often to update, how often to print
@dataclass
class TrainConfig:
	iterations: int = 20
	rollout_steps: int = 64
	max_episode_steps: int = 200
	batch_size: int = 64
	gamma: float = 0.99
	log_every: int = 1


# the whole setup in one object: the trainer takes this and nothing else
@dataclass
class RLConfig:
	env: EnvConfig = field(default_factory=EnvConfig)
	train: TrainConfig = field(default_factory=TrainConfig)
	obs_image_keys: tuple = IMAGE_KEYS
	obs_state_keys: tuple = STATE_KEYS
	seed: int = 0
