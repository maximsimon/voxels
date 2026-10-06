#!/usr/bin/env python3
"""
OBSERVATION FEATURES - splits one stacked observation dict into the two things a policy actually wants: pixel frames with their spatial layout intact, and one small flat vector of scalars.
This is the boundary that keeps the workflow independent of the simulator. Pick keys here once and every policy, buffer and algorithm downstream works on fixed-shape arrays instead of a dict.
The split exists for a reason: the pixel branch keeps its uint8 dtype, because casting a 128x80x3 frame to float32 in the buffer costs 4x the memory and 4x the copying for no gain. The network does the /255 normalisation itself, inside the first layer, where it belongs.
"""
from __future__ import annotations

import numpy as np


# the dtype the pixel branch must arrive in, because the buffer allocates its arrays from it once and never reallocates
IMAGE_DTYPE = np.uint8


# fixed projection from an observation dict to a (uint8 pixel frames, float32 state vector) pair; the layout is frozen by the first observation it sees
class ObsFeatures:
	def __init__(self, image_keys, state_keys):
		if not image_keys or not state_keys:
			raise ValueError("ObsFeatures needs at least one image key and at least one state key")
		self._image_keys = tuple(image_keys)
		self._state_keys = tuple(state_keys)
		self._image_shapes = None
		self._state_shapes = None

	# produce both branches for the current batch of environments
	def __call__(self, obs):
		return self._build_images(obs), self._build_states(obs)

	# fetch one observation entry, failing loudly rather than silently dropping a branch
	def _pick(self, obs, key):
		if key not in obs:
			raise KeyError(f"observation key '{key}' is missing, available: {sorted(obs)}")
		value = obs[key]
		if value is None:
			raise ValueError(f"observation key '{key}' is None, cannot build features from it")
		return value

	# the pixel branch: native dtype preserved, per-env shape untouched, several keys stacked on the channel axis
	def _build_images(self, obs) -> np.ndarray:
		parts = [np.asarray(self._pick(obs, key)) for key in self._image_keys]
		shapes = [part.shape[1:] for part in parts]

		if self._image_shapes is None:
			self._image_shapes = shapes
		elif shapes != self._image_shapes:
			raise ValueError(f"pixel observation layout changed mid-run: {self._image_shapes} -> {shapes}")

		if len(parts) == 1:
			return np.ascontiguousarray(parts[0])
		return np.ascontiguousarray(np.concatenate(parts, axis=-1))

	# the state branch: every selected key flattened and concatenated into one float32 vector per environment
	def _build_states(self, obs) -> np.ndarray:
		parts = [np.asarray(self._pick(obs, key), dtype=np.float32) for key in self._state_keys]
		shapes = [part.shape[1:] for part in parts]

		if self._state_shapes is None:
			self._state_shapes = shapes
		elif shapes != self._state_shapes:
			raise ValueError(f"state observation layout changed mid-run: {self._state_shapes} -> {shapes}")

		flat = np.concatenate([part.reshape(part.shape[0], -1) for part in parts], axis=1)
		return np.ascontiguousarray(flat, dtype=np.float32)

	# shape of one pixel frame without its batch dimension, ready to become the first layer's input shape
	@property
	def image_shape(self) -> tuple:
		if self._image_shapes is None:
			raise RuntimeError("feature layout is not known yet - call the extractor on one observation first")
		if len(self._image_shapes) == 1:
			return tuple(self._image_shapes[0])
		return tuple(self._image_shapes[0][:-1]) + (sum(shape[-1] for shape in self._image_shapes),)

	# length of one state vector, valid only after the first call has frozen the layout
	@property
	def state_dim(self) -> int:
		if self._state_shapes is None:
			raise RuntimeError("feature layout is not known yet - call the extractor on one observation first")
		return int(sum(int(np.prod(shape)) for shape in self._state_shapes))

	# how much each branch costs and which key contributed what, for logging
	@property
	def layout(self) -> dict:
		if self._image_shapes is None or self._state_shapes is None:
			raise RuntimeError("feature layout is not known yet - call the extractor on one observation first")
		image_bytes = int(np.prod(self.image_shape)) * np.dtype(IMAGE_DTYPE).itemsize
		return dict(
			images=self.image_shape,
			states={key: int(np.prod(shape)) for key, shape in zip(self._state_keys, self._state_shapes)},
			state_dim=self.state_dim,
			image_bytes_per_env=image_bytes,
		)
