#!/usr/bin/env python3
"""
REWARD - the only place that decides what a good simulation frame is worth. The reward is a plain function, not a named option: the simulator pipeline calls it once per frame and gets one float per environment back.

Contract, the only thing the rest of the setup relies on:

	def reward_fn(obs, actions) -> np.ndarray   # obs is the stacked observation dict, actions is [num_envs, 6], the result is [num_envs] float32

Rewrite one of the functions below or add your own and point rl_main.REWARD at it, nothing else has to change.

PLACEHOLDER: both functions here are deliberately trivial - pay for forward progress, or pay for not scraping a wall - so the workflow can be checked before a real task is defined.
"""
from __future__ import annotations

import numpy as np

# weight on the achieved forward velocity, tune the scale of the whole reward here
FORWARD_WEIGHT = 1.0
# dock per rad/s of yaw spin, zero keeps the signal as plain "how fast did you get somewhere"
SPIN_PENALTY = 0.0
# constant paid for every frame, on top of whatever the frame itself earned
ALIVE_REWARD = 0.0
# docked on a frame where forward motion was requested but did not happen, a crude stand-in for scraping a wall
BLOCKED_PENALTY = 1.0
# velocities below this count as not moving, in m/s
MOVING_THRESHOLD = 0.01


# PLACEHOLDER REWARD - pay for achieved forward velocity and dock a little for spinning on the spot. The easiest signal to sanity check by eye, since the score tracks how fast the robot drives.
def forward_velocity_reward(obs: dict, actions: np.ndarray) -> np.ndarray:
	forward = obs["linear_vel"][:, 0]
	spin = np.abs(obs["angular_vel"][:, 2])
	reward = FORWARD_WEIGHT * forward - SPIN_PENALTY * spin + ALIVE_REWARD
	return np.asarray(reward, dtype=np.float32)


# PLACEHOLDER REWARD - a small constant reward for every frame, minus a penalty when the robot is told to go forward and does not move, which is roughly what hitting a wall feels like
def blocked_velocity_reward(obs: dict, actions: np.ndarray) -> np.ndarray:
	wanted_motion = np.asarray(actions, dtype=np.float32)[:, 0] > MOVING_THRESHOLD
	achieved = np.abs(obs["linear_vel"][:, 0]) < MOVING_THRESHOLD
	blocked = wanted_motion & achieved
	reward = np.where(blocked, ALIVE_REWARD - BLOCKED_PENALTY, ALIVE_REWARD)
	return np.asarray(reward, dtype=np.float32)
