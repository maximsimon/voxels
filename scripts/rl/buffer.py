#!/usr/bin/env python3
"""
ROLLOUT BUFFER - stores one rollout as [rollout_steps, num_envs] arrays, turns the rewards into discounted returns and hands the whole thing out as shuffled minibatches.
This is the hand-off point between data collection and learning: the trainer fills the buffer, the algorithm consumes batches and never sees the simulator.
Observations are kept in two separate arrays rather than one concatenated vector: the pixel frames stay uint8 [rollout_steps, num_envs, H, W, C] and the scalar state stays float32 [rollout_steps, num_envs, state_dim], so a rollout of 64 steps across 4 envs costs about 8 MB instead of the 32 MB a float32 copy of the same frames would.
"""
from __future__ import annotations

import numpy as np

from obs_features import IMAGE_DTYPE


# one episode-aligned rollout, ready to be sampled by an algorithm
class RolloutBuffer:
	def __init__(self, rollout_steps: int, num_envs: int, image_shape, state_dim: int, action_dim: int,
				 gamma: float = 0.99, seed: int = 0):
		if rollout_steps < 1 or num_envs < 1:
			raise ValueError("rollout_steps and num_envs must both be >= 1")
		self.rollout_steps = int(rollout_steps)
		self.num_envs = int(num_envs)
		self.image_shape = tuple(int(v) for v in image_shape)
		self.state_dim = int(state_dim)
		self.action_dim = int(action_dim)
		self.gamma = float(gamma)
		self._rng = np.random.default_rng(seed)
		self._count = 0
		self._returns_valid = False
		self._alloc()

	# preallocate every array once, so filling a rollout never reallocates
	def _alloc(self) -> None:
		shape = (self.rollout_steps, self.num_envs)
		self.images = np.zeros((*shape, *self.image_shape), dtype=IMAGE_DTYPE)
		self.states = np.zeros((*shape, self.state_dim), dtype=np.float32)
		self.actions = np.zeros((*shape, self.action_dim), dtype=np.float32)
		self.rewards = np.zeros(shape, dtype=np.float32)
		self.dones = np.zeros(shape, dtype=np.float32)
		self.returns = np.zeros(shape, dtype=np.float32)

	@property
	def full(self) -> bool:
		return self._count >= self.rollout_steps

	@property
	def count(self) -> int:
		return self._count

	# how many transitions one full rollout holds
	@property
	def size(self) -> int:
		return self.rollout_steps * self.num_envs

	# drop the current rollout and start a fresh one
	def clear(self) -> None:
		self._count = 0
		self._returns_valid = False

	# append one vectorized frame: everything for all environments at a single point in time
	def add(self, images, states, actions, rewards, dones) -> None:
		if self.full:
			raise RuntimeError(f"rollout buffer is full ({self.rollout_steps} steps), call clear() before filling it again")

		images = np.asarray(images)
		states = np.asarray(states, dtype=np.float32)
		actions = np.asarray(actions, dtype=np.float32)
		rewards = np.asarray(rewards, dtype=np.float32).reshape(-1)
		dones = np.asarray(dones, dtype=bool).reshape(-1)

		if images.shape != (self.num_envs, *self.image_shape):
			raise ValueError(f"images must have shape {(self.num_envs, *self.image_shape)}, got {images.shape}")
		if images.dtype != IMAGE_DTYPE:
			raise ValueError(f"pixel frames must stay {np.dtype(IMAGE_DTYPE).name} to keep the buffer small, got {images.dtype} - normalise inside the policy instead")
		if states.shape != (self.num_envs, self.state_dim):
			raise ValueError(f"states must have shape {(self.num_envs, self.state_dim)}, got {states.shape}")
		if actions.shape != (self.num_envs, self.action_dim):
			raise ValueError(f"actions must have shape {(self.num_envs, self.action_dim)}, got {actions.shape}")
		if rewards.shape != (self.num_envs,) or dones.shape != (self.num_envs,):
			raise ValueError(f"rewards and dones must both have shape ({self.num_envs},)")

		t = self._count
		self.images[t] = images
		self.states[t] = states
		self.actions[t] = actions
		self.rewards[t] = rewards
		self.dones[t] = dones.astype(np.float32)
		self._count += 1
		self._returns_valid = False

	# walk the rollout backwards so a finished episode zeroes the running return at its boundary, then discount
	def compute_returns(self) -> np.ndarray:
		if not self.full:
			raise RuntimeError("cannot compute returns on a rollout that is not full yet")

		running = np.zeros(self.num_envs, dtype=np.float32)
		for t in range(self._count - 1, -1, -1):
			alive = 1.0 - self.dones[t]
			running = self.rewards[t] + self.gamma * running * alive
			self.returns[t] = running
		self._returns_valid = True
		return self.returns

	# validate the rollout and then hand back an iterator over shuffled minibatches, kept apart so the checks run at call time rather than on first next()
	def batches(self, batch_size: int):
		if not self.full:
			raise RuntimeError("cannot sample from a rollout that is not full yet")
		if not self._returns_valid:
			self.compute_returns()
		return self._iter_batches(batch_size)

	# flatten the rollout into (rollout_steps * num_envs) transitions and chop it into shuffled minibatches
	def _iter_batches(self, batch_size: int):
		count = self._count * self.num_envs
		batch_size = max(1, min(int(batch_size), count))

		images = self.images[:self._count].reshape((count, *self.image_shape))
		states = self.states[:self._count].reshape(count, self.state_dim)
		actions = self.actions[:self._count].reshape(count, self.action_dim)
		rewards = self.rewards[:self._count].reshape(count)
		returns = self.returns[:self._count].reshape(count)
		dones = self.dones[:self._count].reshape(count)

		order = self._rng.permutation(count)
		for start in range(0, count, batch_size):
			index = order[start:start + batch_size]
			yield dict(
				images=images[index],
				states=states[index],
				actions=actions[index],
				rewards=rewards[index],
				returns=returns[index],
				dones=dones[index],
			)

	# cheap rollout-level statistics, for logging rather than for learning
	def summary(self) -> dict:
		if self._count == 0:
			return dict(reward_mean=0.0, return_mean=0.0, done_rate=0.0)
		if not self._returns_valid:
			self.compute_returns()
		frames = self._count * self.num_envs
		return dict(
			reward_mean=float(self.rewards[:self._count].sum() / frames),
			return_mean=float(self.returns[:self._count].sum() / frames),
			done_rate=float(self.dones[:self._count].sum() / frames),
		)
