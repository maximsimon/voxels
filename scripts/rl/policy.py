#!/usr/bin/env python3
"""
POLICY - the only place allowed to turn an observation into an action. The trainer never inspects how an action was produced, it only calls act().
The observation arrives as two branches, and a real network wants exactly that: pixel frames to a conv tower, the small state vector alongside it, and their outputs joined before the action head.
Swap the behaviour by pointing rl_main.POLICY at a different class, no other module has to be touched. Any class with the same constructor arguments and an act() method will do.

PLACEHOLDER: both policies below are deliberately trivial (uniform noise and a constant drive command) so the workflow can be verified end to end before any learning maths goes in.
"""
from __future__ import annotations

import numpy as np

# base class every policy derives from: image frames and a state vector in, a [num_envs, action_dim] action batch out
class Policy:
	def __init__(self, num_envs: int, action_dim: int, image_shape, state_dim: int, seed: int = 0):
		self.num_envs = num_envs
		self.action_dim = action_dim
		self.image_shape = tuple(image_shape)
		self.state_dim = state_dim
		self._rng = np.random.default_rng(seed)

	# produce one action per environment from the current pixel frames and state vectors
	def act(self, images: np.ndarray, states: np.ndarray, step: int) -> np.ndarray:
		raise NotImplementedError("every policy must implement act()")

	# called once per rollout, the hook where a stateful policy (recurrent memory, running normalization) resets itself
	def on_rollout_start(self) -> None:
		pass

	# sanity-check that an action batch is shaped the way the simulator demands
	def _check(self, actions: np.ndarray) -> np.ndarray:
		actions = np.asarray(actions, dtype=np.float32)
		if actions.shape != (self.num_envs, self.action_dim):
			raise ValueError(f"policy produced actions of shape {actions.shape}, expected {(self.num_envs, self.action_dim)}")
		return actions


# PLACEHOLDER POLICY - uniform noise on every axis, the simplest possible thing that still exercises the whole loop
class RandomPolicy(Policy):
	def __init__(self, num_envs: int, action_dim: int, image_shape, state_dim: int, seed: int = 0, action_scale: float = 1.0):
		super().__init__(num_envs, action_dim, image_shape, state_dim, seed)
		self._scale = float(action_scale)

	def act(self, images: np.ndarray, states: np.ndarray, step: int) -> np.ndarray:
		return self._check(self._rng.uniform(-self._scale, self._scale, size=(self.num_envs, self.action_dim)))


# PLACEHOLDER POLICY - one fixed action broadcast to every environment, useful as a scripted baseline to compare a learned policy against
class ConstantPolicy(Policy):
	def __init__(self, num_envs: int, action_dim: int, image_shape, state_dim: int, seed: int = 0, action=None):
		super().__init__(num_envs, action_dim, image_shape, state_dim, seed)
		action = np.zeros(action_dim, dtype=np.float32) if action is None else np.asarray(action, dtype=np.float32)
		if action.shape != (action_dim,):
			raise ValueError(f"a constant action must have shape ({action_dim},), got {action.shape}")
		self._action = action

	def act(self, images: np.ndarray, states: np.ndarray, step: int) -> np.ndarray:
		return self._check(np.tile(self._action, (self.num_envs, 1)))
