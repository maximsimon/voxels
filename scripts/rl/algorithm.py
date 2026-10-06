#!/usr/bin/env python3
"""
ALGORITHM - the learner step, the only part of the setup that is supposed to change anything. It receives minibatches of transitions and returns a dict of scalars for the logger.
A batch carries both observation branches, pixel frames as uint8 and the state vector as float32, so a learner can feed a conv tower and the scalar branch separately without going back to the simulator.
Nothing in here knows about the simulator, the observation keys, the policy internals or the reward formula: swap this file, or point rl_main.ALGORITHM at a different class, and the rest of the workflow keeps running.

PLACEHOLDER: the learner below does not learn anything yet, it only measures the batch, which is enough to prove the collect-compute-update-log pipeline is wired correctly.
"""
from __future__ import annotations

import numpy as np


# base class every algorithm derives from: one call per minibatch, one dict of floats back
class Algorithm:
	def __init__(self, image_shape, state_dim: int, action_dim: int, num_envs: int, seed: int = 0):
		self.image_shape = tuple(image_shape)
		self.state_dim = int(state_dim)
		self.action_dim = int(action_dim)
		self.num_envs = int(num_envs)
		self._rng = np.random.default_rng(seed)

	# consume every minibatch of one rollout and merge the per-batch scalars into one dict
	def update(self, buffer) -> dict:
		merged = None
		n_batches = 0
		for batch in buffer.batches(self.batch_size):
			metrics = self._update_batch(batch)
			merged = metrics if merged is None else {k: merged[k] + v for k, v in metrics.items()}
			n_batches += 1

		if merged is None:
			raise RuntimeError("algorithm received an empty rollout")
		return {k: v / n_batches for k, v in merged.items()}

	# fit a single minibatch and report on it
	def _update_batch(self, batch: dict) -> dict:
		raise NotImplementedError("every algorithm must implement _update_batch()")

	# how many transitions the base workflow hands over at a time
	@property
	def batch_size(self) -> int:
		return 256


# PLACEHOLDER ALGORITHM - reports batch statistics and touches nothing, so the workflow can be validated before any real learning rule lands
class PlaceholderAlgorithm(Algorithm):
	def __init__(self, image_shape, state_dim: int, action_dim: int, num_envs: int, seed: int = 0, batch_size: int = 256):
		super().__init__(image_shape, state_dim, action_dim, num_envs, seed)
		self._batch_size = int(batch_size)

	@property
	def batch_size(self) -> int:
		return self._batch_size

	def _update_batch(self, batch: dict) -> dict:
		returns = np.asarray(batch["returns"], dtype=np.float32)
		centered = returns - returns.mean()
		return dict(
			loss=0.0,
			batch_return_mean=float(returns.mean()),
			batch_return_std=float(returns.std()),
			advantage_mean=float(centered.mean()),
			# a mean pixel brightness of zero means the camera branch is arriving empty, which is invisible in every other metric
			image_mean=float(np.asarray(batch["images"], dtype=np.float32).mean()),
		)
