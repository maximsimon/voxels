#!/usr/bin/env python3
"""
TRAINER - the generic reinforcement learning workflow, and the only module that talks to the simulator. Reset the environments, collect rollouts with the policy, let the algorithm consume the batches, print the numbers, repeat.
Nothing in here knows what the robot should do or what a good frame is worth: the policy class and the reward function arrive as arguments, so the same loop drives any combination of them.
The trainer is also the assembler, which is why it builds the policy and the algorithm itself: by the time they exist the shapes are known, because the feature layout is frozen by the first reset and the action width comes from the VecEnv.
"""
from __future__ import annotations

import time

import numpy as np

from buffer import RolloutBuffer
from obs_features import ObsFeatures


# drives the collect-compute-update loop over a VecEnv, one object per run
class Trainer:
	def __init__(self, env, policy_cls, reward_fn, algorithm_cls, config):
		self.env = env
		self.policy_cls = policy_cls
		self.reward_fn = reward_fn
		self.algorithm_cls = algorithm_cls
		self.config = config
		self.features = ObsFeatures(config.obs_image_keys, config.obs_state_keys)
		self._rng = np.random.default_rng(config.seed)
		self.policy = None
		self.algorithm = None
		self.buffer = None
		self._episode_length = None
		self._observation = None

	# hook the reward function into the simulator pipeline, so VecEnv.step() scores every frame on its own
	def _install_reward(self) -> None:
		self.env.reward_fn = self.reward_fn

	# scatter the robots around the spawn area with random headings, so a rollout does not start with N identical copies
	def _spawn_poses(self) -> list:
		radius = self.config.env.spawn_radius
		return [(float(self._rng.uniform(-radius, radius)),
				 float(self._rng.uniform(-radius, radius)),
				 float(self._rng.uniform(0.0, 360.0))) for _ in range(self.env.n_envs)]

	# copy one freshly reset observation back into the stacked batch, so the next rollout frame starts the new episode for that environment
	def _splice(self, observation, idx: int, single: dict) -> None:
		for key, value in observation.items():
			if value is None or single.get(key) is None:
				continue
			value[idx] = single[key]

	# reset the environments, freeze the observation layout, then build the policy and the algorithm against it
	def setup(self) -> dict:
		self._install_reward()

		self._worlds = tuple()
		if self.config.env.world_folder:
			self._worlds = tuple(self.env.load_worlds(self.config.env.world_folder))

		initial_worlds = self._initial_worlds()
		self._current_world = list(initial_worlds)
		self._observation = self.env.reset(initial_worlds, self._spawn_poses())

		images, _ = self.features(self._observation)
		image_shape = images.shape[1:]
		state_dim = self.features.state_dim
		action_dim = self.env.action_shape[0]

		self.policy = self.policy_cls(self.env.num_envs, action_dim, image_shape, state_dim, seed=self.config.seed)
		self.algorithm = self.algorithm_cls(image_shape, state_dim, action_dim, self.env.num_envs, seed=self.config.seed,
											batch_size=self.config.train.batch_size)

		self._episode_length = np.zeros(self.env.num_envs, dtype=np.int64)
		self.buffer = RolloutBuffer(self.config.train.rollout_steps, self.env.num_envs, image_shape, state_dim, action_dim,
									gamma=self.config.train.gamma, seed=self.config.seed)

		return dict(image_shape=image_shape, state_dim=state_dim, action_dim=action_dim, layout=self.features.layout,
					worlds=self._worlds)

	# the world each env starts its very first episode in - a random one from the loaded pool, or the single configured world repeated when no pool is loaded
	def _initial_worlds(self) -> list:
		if not self._worlds:
			return [self.config.env.world] * self.env.num_envs
		return [self._rng.choice(self._worlds) for _ in range(self.env.num_envs)]

	# a world for an env that has just finished an episode: never the one it just left, so every episode is a new map. with a single world in the pool there is nothing else to pick and that one is reused.
	def _next_world(self, idx: int):
		if not self._worlds:
			return self.config.env.world
		choices = [w for w in self._worlds if w != self._current_world[idx]]
		return self._rng.choice(choices) if choices else self._rng.choice(self._worlds)

	# collect one rollout worth of transitions from every environment in lockstep
	def collect_rollout(self) -> dict:
		self.buffer.clear()
		self.policy.on_rollout_start()
		poses = self._spawn_poses()
		episode_rewards = np.zeros(self.env.num_envs, dtype=np.float32)
		episodes_finished = 0
		time_limits = 0

		for step in range(self.config.train.rollout_steps):
			images, states = self.features(self._observation)
			actions = self.policy.act(images, states, step)
			observation, rewards, terminated, truncated, _ = self.env.step(actions)

			self._episode_length += 1
			out_of_time = self._episode_length >= self.config.train.max_episode_steps
			# a time limit cuts the episode like a real termination does, which is exact here because nothing is bootstrapped yet - revisit this the moment a value function exists
			finished = np.asarray(terminated, dtype=bool) | np.asarray(truncated, dtype=bool) | out_of_time
			time_limits += int(out_of_time.sum())

			self.buffer.add(images, states, actions, rewards, finished)
			episode_rewards += rewards
			self._observation = observation

			finished_envs = np.flatnonzero(finished).tolist()
			if finished_envs:
				episodes_finished += len(finished_envs)
				worlds = [self._next_world(int(idx)) for idx in finished_envs]
				# one pipelined call, not one call per env: a world switch rebuilds the world, and doing them concurrently costs a single rebuild in wall time
				resets = self.env.reset_many(finished_envs, worlds, [poses[int(idx)] for idx in finished_envs])
				for idx, world, single in zip(finished_envs, worlds, resets):
					self._current_world[idx] = world
					self._splice(self._observation, int(idx), single)
					self._episode_length[idx] = 0

		return dict(episodes_finished=episodes_finished, time_limits=time_limits,
					episode_reward_mean=float(episode_rewards.mean()))

	# run the configured number of iterations, logging one line each, and hand back the last set of numbers
	def run(self) -> dict:
		info = self.setup()
		last = dict()
		layout = info["layout"]

		print(f"pixel branch : {layout['images']} uint8, {layout['image_bytes_per_env'] / 1024:.1f} KB per env")
		print(f"state branch : {info['state_dim']} floats per env {layout['states']}")
		if info["worlds"]:
			print(f"world pool   : {len(info['worlds'])} worlds from {self.config.env.world_folder}, a new one per episode")
		else:
			print(f"world        : {self.config.env.world} for every episode")
		print(f"rollout      : {self.config.train.rollout_steps} steps x {self.env.num_envs} envs "
			  f"= {self.buffer.size} transitions, {self.buffer.images.nbytes / 1e6:.1f} MB of pixels per iteration")
		print("iteration    reward     return     episodes    steps/s")

		start = time.perf_counter()
		for iteration in range(1, self.config.train.iterations + 1):
			rollout_stats = self.collect_rollout()
			self.buffer.compute_returns()

			metrics = self.algorithm.update(self.buffer)
			summary = self.buffer.summary()

			elapsed = time.perf_counter() - start
			rate = (iteration * self.buffer.size) / max(elapsed, 1e-9)
			last = dict(iteration=iteration, elapsed=elapsed, steps_per_second=rate,
						**summary, **metrics, **rollout_stats)

			if self.config.train.log_every > 0 and iteration % self.config.train.log_every == 0:
				print(f"{iteration:>9d}  {summary['reward_mean']:>9.4f}  {summary['return_mean']:>9.4f}  "
					  f"{rollout_stats['episodes_finished']:>9d}  {rate:>8.1f}")

		return last
