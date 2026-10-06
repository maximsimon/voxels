#!/usr/bin/env python3
"""
Vectorized (parallel) environments for the Voxel World simulator.

Runs N independent simulator processes side by side and exposes a gymnasium-style batched API: reset() returns observations stacked along a leading batch dimension [N, ...], step(actions) accepts a [N, 6] action batch and returns (obs, reward, terminated, truncated, info).

Two separate window switches, deliberately not sharing a name: show_mosaic opens the single parent-side window that tiles every instance side by side, while the per-env config's show_window controls an individual simulator's own raylib window and is forced off whenever more than one instance runs.

"""
from __future__ import annotations

import glob
import multiprocessing as mp
import os

import numpy as np

import voxel_sim  # noqa: F401  (imported by every worker, and by pickling)

from wrapper import WrapperWindow

# configuration parameters
DEFAULT_ENV_CONFIG = dict(
	verbose=False,          # silence raylib INFO spam + world-build chatter
	target_fps=0,           # no frame-rate cap
	show_window=False,      # never map the per-sim window (a VecEnv with more than one env forces this off anyway)
	render_gui=False,       # skip the third-person pass
	keyboard_enabled=False, # keys cannot perturb a rollout
	quit_key_enabled=False,
	screen_width=1600,
	screen_height=1600,
	camera_width=1280,
	camera_height=800,
	lidar_enabled=True,
	lidar_rays=180,
	lidar_range=40.0,
	draw_lidar_rays=False,
	fast_collisions=True,
	collision_radius=0.5,
	max_linear_speed=0.0,
	max_angular_speed=0.0,
	fixed_dt=0.02,
)


# worker of a process, worker is one instance of the actual simulation (voxel world) - One simulated environment, one process.  Owns its own raylib window.
def _worker(idx: int, conn, config: dict) -> None:
	try:
		voxel_sim.Configure(**config)
		obs = voxel_sim.Init()
		conn.send(("ready", obs))

		while True:
			cmd = conn.recv()
			if cmd[0] == "step":
				obs = voxel_sim.Step(cmd[1])
				conn.send(("obs", obs))
			elif cmd[0] == "reset":
				obs = voxel_sim.Reset(cmd[1], cmd[2])
				conn.send(("obs", obs))
			elif cmd[0] == "load_registry":
				# the world table lives inside each worker, so a registry has to be loaded per process before its worlds can be named
				voxel_sim.ReloadRegistry(cmd[1])
				conn.send(("worlds", list(voxel_sim.WORLDS)))
			elif cmd[0] == "close":
				voxel_sim.Close()
				conn.send(("done", None))
				break
			else:
				raise ValueError(f"worker recieved unknown command cmd[0]: {cmd[0]!r}")

	# error handling (redundance ..?)
	except Exception as exc:  # noqa: BLE001 - surface any worker failure to the parent
		try:
			conn.send(("error", f"{type(exc).__name__}: {exc}"))
		except Exception:
			pass

	
# stack observation dicts from each env into one dict for vec-env (of [N, ...] arrays)
def _stack_observations(obses) -> dict:
	first = obses[0]
	out = {}
	for key in first:
		values = [d[key] for d in obses]

		if key in ("camera_front",) and any(v is None for v in values):
			out[key] = None
			continue

		if isinstance(values[0], np.ndarray):
			arr = np.stack(values, axis=0)
		else:
			arr = np.asarray(values)
		if arr.dtype == np.float64:
			arr = arr.astype(np.float32)
		out[key] = arr
	return out

# a folder of generated worlds is pointed at by the registry file inside it - one folder, one *.registry, since two would be ambiguous about which world set is meant
def _resolve_registry(source) -> str:
	path = os.path.expanduser(str(source))

	if os.path.isdir(path):
		found = sorted(glob.glob(os.path.join(path, "*.registry")))
		if not found:
			raise FileNotFoundError(f"no *.registry file in '{path}' - a world folder needs one")
		if len(found) > 1:
			raise ValueError(f"'{path}' holds {len(found)} registries {found} - point at the one you want")
		path = found[0]
	elif not os.path.isfile(path):
		raise FileNotFoundError(f"no such world folder or registry: '{source}'")

	return path


# the actual class for paralelization of N running RL environements, contains stuff like step(), reset()
# all observation keys come back with a leading [N, ...] batch dimension.
class VecEnv:
	def __init__(self, n_envs: int, config=None, worlds=None, start_method: str = "fork", show_mosaic=False):
		# TODO: handle start_method = fork --- either make it gone, hardocded, or put into config or something		

		if n_envs < 1:
			raise ValueError("n_envs must be >= 1")

		# handle configs: normalize per-env configs to a list of N dicts.
		# TODO: put config into seperate config file (mby under config folders)
		if config is None:									# no config sent		
			configs = [dict(DEFAULT_ENV_CONFIG) for _ in range(n_envs)]
		elif isinstance(config, dict):								# one config sent
			base = dict(DEFAULT_ENV_CONFIG)
			base.update(config)
			configs = [dict(base) for _ in range(n_envs)]
		elif isinstance(config, (list, tuple)):							# one config per environtment (so N configs) sent
			if len(config) != n_envs:
				raise ValueError("a per-env config list must have one entry per env")
			configs = [dict(DEFAULT_ENV_CONFIG) if c is None else dict(DEFAULT_ENV_CONFIG, **c)
					   for c in config]
		else:
			raise TypeError("config must be a dict, a list of dicts, or None")

		# more than one instance and the per-sim "You're in voxels now" windows stay off by default: N of them would fight each other and the mosaic for the same screen. The sims still render, the mosaic is built from their camera frames, so nothing is lost. Run a single instance if that window is what you want.
		if n_envs > 1:
			for env_config in configs:
				env_config["show_window"] = False

		self.n_envs = n_envs
		self._action_dim = 6

		try:
			self._ctx = mp.get_context(start_method)
		except ValueError as exc:
			raise ValueError(f"bad start method {start_method!r}") from exc

		self._conns = []
		self._procs = []
		self._closed = False
		self._wrapper = None
		self._worlds = ()

		# create the vector of environemtns (vec-env): start gym-style env N times
		try:
			for i in range(n_envs):
				parent_conn, child_conn = self._ctx.Pipe(duplex=True)
				proc = self._ctx.Process(target=_worker, args=(i, child_conn, configs[i]))		# start worker (which starts the simulation instance)
				proc.daemon = True
				proc.start()
				child_conn.close()          # parent side keeps only its end
				self._conns.append(parent_conn)
				self._procs.append(proc)

			# handle errors: wait until every worker has finished Init() (a hidden window + world build) before returning
			for i in range(n_envs):
				try:
					msg = self._conns[i].recv()
				except (EOFError, OSError) as exc:
					raise RuntimeError(f"env {i} worker died during init") from exc
				if msg[0] == "error":
					raise RuntimeError(f"env {i} failed to initialize: {msg[1]}")
				if msg[0] != "ready":
					raise RuntimeError(f"env {i}: unexpected message {msg[0]!r}")

			# wrapper lives in the parent, after workers forked (no Qt state in children)
			# the mosaic lives in the parent and is the one window a parallel run shows
			if show_mosaic:
				self._wrapper = WrapperWindow(self.n_envs)
		except Exception:
			self.close()
			raise

	# -- private plumbing ----------------------------------------------------

	# feed the parent-side wrapper window (h=hide, s=show) from the batched obs
	def _maybe_render(self, obs) -> None:
		if self._wrapper is not None:
			self._wrapper.update(obs.get("camera_front"))

	# send command to 1 env and recieve msg response - used by reset_at() and close()
	def _send_recv(self, idx: int, cmd):
		try:
			self._conns[idx].send(cmd)
			msg = self._conns[idx].recv()
		except (EOFError, OSError) as exc:
			raise RuntimeError(f"env {idx} worker died") from exc
		if msg[0] == "error":
			raise RuntimeError(f"env {idx}: {msg[1]}")
		return msg

	
	# send (push) commands to all envs in vec-env and then recieve all responses (every msg from every env at once) 
	# this is what makes the vector call parallel: all N workers receive their action before any reply is awaited, so they step concurrently on their own cores
	def _send_all_recv_all(self, cmds):
		for i, cmd in enumerate(cmds):
			try:
				self._conns[i].send(cmd)
			except (EOFError, OSError) as exc:
				raise RuntimeError(f"env {i} worker died") from exc

		replies = []
		for i in range(self.n_envs):
			try:
				msg = self._conns[i].recv()
			except (EOFError, OSError) as exc:
				raise RuntimeError(f"env {i} worker died") from exc
			if msg[0] == "error":
				raise RuntimeError(f"env {i}: {msg[1]}")
			if msg[0] not in ("obs", "done", "ready", "worlds"):
				raise RuntimeError(f"env {i}: unexpected message {msg[0]!r}")
			replies.append(msg[1])
		return replies

	# same pipelined round trip as _send_all_recv_all, but only for the listed envs - lets several finished episodes rebuild their worlds concurrently instead of one after another
	def _send_many_recv_many(self, indices, cmds):
		for i, cmd in zip(indices, cmds):
			try:
				self._conns[i].send(cmd)
			except (EOFError, OSError) as exc:
				raise RuntimeError(f"env {i} worker died") from exc

		replies = []
		for i in indices:
			try:
				msg = self._conns[i].recv()
			except (EOFError, OSError) as exc:
				raise RuntimeError(f"env {i} worker died") from exc
			if msg[0] == "error":
				raise RuntimeError(f"env {i}: {msg[1]}")
			if msg[0] != "obs":
				raise RuntimeError(f"env {i}: unexpected message {msg[0]!r}")
			replies.append(msg[1])
		return replies

	

	# -- public API ----------------------------------------------------------

	# to fetch number of envs in vec-env (e.g. usage: env = VecEnv(8), env.num_envs returns 8)
	@property
	def num_envs(self) -> int:
		return self.n_envs

	# to fetch action shape (e.g. usage:  env = VecEnv(), env.action_shape)
	@property
	def action_shape(self):
		return (self._action_dim,)
	
	# fetch if simulation is running or not
	@property
	def is_closed(self) -> bool:
		return self._closed	
	# reset all environments, start new episode in each env with specified world and position for each, returns stacked observations
	def reset(self, worlds=None, positions=None) -> dict:
		if self._closed:
			raise RuntimeError("VecEnv is closed")

		worlds = self._norm_worlds(worlds)
		positions = self._norm_positions(positions)

		obses = self._send_all_recv_all([("reset", worlds[i], positions[i]) for i in range(self.n_envs)])

		# camera resolution and lidar length must agree across envs for stacking
		cam_shapes = [o["camera_front"].shape for o in obses if o["camera_front"] is not None]
		if cam_shapes and len(set(cam_shapes)) != 1:
			raise ValueError(f"camera resolutions differ across envs: {set(cam_shapes)} - configure them uniformly")
		lidar_lens = {o["lidar_scan"].shape[0] for o in obses}
		if len(lidar_lens) != 1:
			raise ValueError(f"lidar_rays differ across envs: {lidar_lens}")

		obs = _stack_observations(obses)
		self._maybe_render(obs)
		return obs

	# step all environments, returns (stacked_obs, rewards, terminated, truncated, info) all lists of N elements (for each env in vec-env)
	# truncated is in for gym-style RL convencmtion
	def step(self, actions):
		if self._closed:
			raise RuntimeError("VecEnv is closed")

		acts = np.asarray(actions, dtype=np.float32)
		if acts.ndim != 2 or acts.shape[0] != self.n_envs or acts.shape[1] != self._action_dim:
			raise ValueError(f"actions must have shape ({self.n_envs}, {self._action_dim}), got {acts.shape}")

		obses = self._send_all_recv_all([("step", acts[i]) for i in range(self.n_envs)])
		obs = _stack_observations(obses)

		terminated = np.array([not o["running"] for o in obses], dtype=bool)
		reward = np.zeros(self.n_envs, dtype=np.float32)
		reward_fn = getattr(self, "reward_fn", None)
		if reward_fn is not None:
			reward = np.asarray(reward_fn(obs, acts), dtype=np.float32)
			if reward.shape != (self.n_envs,):
				raise ValueError("reward_fn must return an array of shape (n_envs,)")
		truncated = np.zeros(self.n_envs, dtype=bool)
		info = [{} for _ in range(self.n_envs)]

		self._maybe_render(obs)
		return obs, reward, terminated, truncated, info

	# reset one environment - returns one (obiously unstacked) observation
	def reset_at(self, idx: int, world=None, position=None) -> dict:
		if self._closed:
			raise RuntimeError("VecEnv is closed")
		if not 0 <= idx < self.n_envs:
			raise IndexError(f"env index {idx} out of range")
		msg = self._send_recv(idx, ("reset", world, position))
		return msg[1]

	# reset a subset of environments in one pipelined round trip, returning their observations in the order asked for. matters whenever several envs finish on the same step: switching worlds rebuilds a world (~1.3 s), so doing them concurrently instead of back to back is the difference between one rebuild and N.
	def reset_many(self, indices, worlds=None, positions=None) -> list:
		if self._closed:
			raise RuntimeError("VecEnv is closed")

		indices = [int(i) for i in indices]
		if not indices:
			return []
		for i in indices:
			if not 0 <= i < self.n_envs:
				raise IndexError(f"env index {i} out of range")
		if len(set(indices)) != len(indices):
			raise ValueError("reset_many got the same env index twice")

		worlds = [None] * len(indices) if worlds is None else list(worlds)
		positions = [None] * len(indices) if positions is None else list(positions)
		if len(worlds) != len(indices) or len(positions) != len(indices):
			raise ValueError("reset_many needs one world and one position per index")

		unknown = [w for w in worlds if w is not None and self._worlds and w not in self._worlds]
		if unknown:
			raise ValueError(f"worlds not in the loaded registry: {unknown} - known: {list(self._worlds)}")

		return self._send_many_recv_many(indices, [("reset", w, p) for w, p in zip(worlds, positions)])

	# replace every worker's world table with the registry in `source` - either a folder holding exactly one *.registry file, or the registry file itself - and return the world names it defines. this REPLACES the table, so worlds from the previous registry (the classic ones by default) stop being known.
	def load_worlds(self, source) -> tuple:
		if self._closed:
			raise RuntimeError("VecEnv is closed")

		path = _resolve_registry(source)
		for i in range(self.n_envs):
			try:
				self._conns[i].send(("load_registry", path))
			except (EOFError, OSError) as exc:
				raise RuntimeError(f"env {i} worker died") from exc

		pools = []
		for i in range(self.n_envs):
			try:
				msg = self._conns[i].recv()
			except (EOFError, OSError) as exc:
				raise RuntimeError(f"env {i} worker died") from exc
			if msg[0] == "error":
				raise RuntimeError(f"env {i}: {msg[1]}")
			pools.append(tuple(msg[1]))

		if any(pool != pools[0] for pool in pools):
			raise RuntimeError(f"workers disagree about the worlds in {path}: {pools}")
		if not pools[0]:
			raise RuntimeError(f"{path} defines no worlds")

		self._worlds = pools[0]
		return self._worlds

	# the world names currently loaded in every worker, empty until load_worlds() runs
	@property
	def worlds(self) -> tuple:
		return self._worlds

	# shut every worker down and release the simulation (raylib windows, gl contexts) 
	def close(self) -> None:
		if self._closed:
			return
		self._closed = True
		for i, conn in enumerate(self._conns):
			try: conn.send(("close",))
			except Exception: pass
		for i in range(self.n_envs):
			try: self._conns[i].recv()
			except Exception: pass
		if self._wrapper is not None:
			self._wrapper.close()
			self._wrapper = None
		for proc in self._procs:
			proc.join(timeout=10)

	# convience for use VecEnv with "with" keyword (with VecEnv() as env: ...here put your gym-style RL code)
	def __enter__(self):
		return self

	def __exit__(self, *exc):
		self.close()


	# -- helpers --------- they transform value into format which is pasable to _send_all_recv_all 

	# normalization of World value (e..g to set one World to all envs) 
	def _norm_worlds(self, value):
		n = self.n_envs
		if value is None:
			return [None] * n
		if isinstance(value, str):
			return [value] * n
		seq = list(value)
		if len(seq) != n:
			raise ValueError(f"_norm_worlds expected {n} world names, got {len(seq)}")
		return seq

	# normalization of pose (e..g to set one pose to all envs all disect list of poses to specify pose in each env) 
	def _norm_positions(self, value):
		n = self.n_envs
		if value is None:
			return [None] * n

		# A structured per-env list: every entry is itself a pose or None.
		if isinstance(value, (list, tuple)) and len(value) == n \
				and value and isinstance(value[0], (list, tuple, type(None))):
			return list(value)

		arr = np.asarray(value, dtype=np.float64)
		if arr.ndim == 1:
			if arr.shape[0] not in (2, 3):
				raise ValueError("_norm_positions: a pose must be (x, z) or (x, z, yaw_deg)")
			return [tuple(arr)] * n
		if arr.ndim == 2:
			if arr.shape[0] != n or arr.shape[1] not in (2, 3):
				raise ValueError(f"_norm_positions expected {n} poses of length 2 or 3, got {arr.shape}")
			return [tuple(row) for row in arr]
		raise ValueError("_norm_positions: positions must be (x, z[, yaw_deg]) or a list of such")
