#!/usr/bin/env python3
"""Informed repeat, parallelized: all taught maps driven at once through VecEnv.

`python_informed_repeat.py` drives the taught trajectories one map at a time, so its
wall time is the sum over the maps.  This version gives every map its own simulator
process (examples/vec_env.py) so all maps repeat concurrently and the wall time is that
of the slowest map instead of the total.  The controller is the same pure pursuit; only
the execution model changes.

Run from the repository root, so the simulator finds its resources:

	PYTHONPATH=build python3 examples/python_informed_repeat_vec.py
	PYTHONPATH=build python3 examples/python_informed_repeat_vec.py --camera-size 128x80
	PYTHONPATH=build python3 examples/python_informed_repeat_vec.py --help

After the vector run it drives the same maps again sequentially in this one process and
prints a wall-time comparison, so the speed-up of the parallel execution is measured on
your machine rather than assumed.  That comparison runs *after* VecEnv.close(): a VecEnv
must be created before this process ever opens a raylib window (forking under an existing
GL context would duplicate it into the workers).

Coordinate/heading conventions are identical to python_informed_repeat.py, including the
sign flip that YAW_RATE_SIGN isolates.
"""

from __future__ import annotations

import argparse
import math
import os
import sys
import time

import numpy as np

_HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, _HERE)                       # so `import vec_env` / `import vtr_map` works
sys.path.insert(0, os.path.dirname(_HERE))      # repo root: resources resolve from cwd anyway

from vec_env import VecEnv  # noqa: E402
from vtr_map import load_map  # noqa: E402
import voxel_sim  # noqa: E402  (used only for the sequential comparison)

MAPS_DIR = os.path.join(_HERE, "maps")

# Sign relating a commanded yaw rate (action[4]) to the heading the simulator reports in
# obs["yaw"].  It is -1, not +1 (see python_informed_repeat.py for the full explanation).
YAW_RATE_SIGN = -1.0


def wrap_pi(a):
	"""Wrap an angle to (-pi, pi]."""
	return (a + math.pi) % (2.0 * math.pi) - math.pi


class ReferencePath:
	"""The taught poses as an arc-length parameterized path."""

	def __init__(self, poses: np.ndarray):
		self.xy = poses[:, :2].astype(np.float64)
		seg = np.hypot(*np.diff(self.xy, axis=0).T)
		self.s = np.concatenate([[0.0], np.cumsum(seg)])
		self.length = float(self.s[-1])
		self.start_yaw = float(poses[0, 2])

	def point_at(self, s: float) -> np.ndarray:
		"""Position at arc length s, linearly interpolated between taught samples."""
		s = min(max(s, 0.0), self.length)
		i = int(np.searchsorted(self.s, s, side="right")) - 1
		i = min(max(i, 0), len(self.s) - 2)
		span = self.s[i + 1] - self.s[i]
		t = 0.0 if span <= 0.0 else (s - self.s[i]) / span
		return self.xy[i] + t * (self.xy[i + 1] - self.xy[i])

	def project(self, p: np.ndarray, s_hint: float, window: float = 8.0) -> tuple:
		"""Arc length of the closest path point, and the distance to it."""
		lo = int(np.searchsorted(self.s, s_hint - window))
		hi = int(np.searchsorted(self.s, s_hint + window)) + 1
		lo, hi = max(lo, 0), min(hi, len(self.xy))
		if hi - lo < 2:
			lo, hi = max(hi - 2, 0), min(lo + 2, len(self.xy))
		d = np.hypot(*(self.xy[lo:hi] - p).T)
		k = int(np.argmin(d))
		return float(self.s[lo + k]), float(d[k])


def new_state(path: ReferencePath, map_name: str, args) -> dict:
	"""Per-environment controller state, one dict per map running concurrently."""
	return {
		"path": path,
		"map": map_name,
		"length": path.length,
		"start": (float(path.xy[0, 0]), float(path.xy[0, 1]), math.degrees(path.start_yaw)),
		"max_steps": int(args.step_budget * path.length * args.steps_per_meter) + 100,
		"s": 0.0,
		"steps": 0,
		"stalled": 0,
		"cmd_forward": 0.0,
		"rew": 0.0,
		"xte": [],
		"pending": [],
		"captured": [],
		"done": False,
	}


def plan_action(st: dict, obs_position, obs_yaw, args) -> np.ndarray:
	"""Pure pursuit for one env, exactly as python_informed_repeat.py does it."""
	p = np.array([obs_position[0], obs_position[2]], dtype=np.float64)
	s, d = st["path"].project(p, st["s"])
	st["s"], st["d"] = s, d
	st["xte"].append(d)

	target = st["path"].point_at(s + args.lookahead)
	heading_err = wrap_pi(math.atan2(target[1] - p[1], target[0] - p[0]) - obs_yaw)

	v = args.speed * max(0.0, 1.0 - abs(heading_err) / args.slow_angle)
	v = max(v, args.min_speed)
	w = max(-args.turn_rate, min(args.turn_rate, args.turn_gain * heading_err))

	# A wall zeroes translation but not rotation.  Once we have pushed into one for a
	# few steps, pivot free instead of grinding against the voxel for the whole budget.
	if st["stalled"] > args.stall_steps:
		v = 0.0
		w = args.turn_rate if heading_err >= 0.0 else -args.turn_rate

	st["cmd_forward"] = v
	return np.array([v, 0.0, 0.0, 0.0, YAW_RATE_SIGN * w, 0.0], dtype=np.float32)


def reward_fn(obs, acts) -> np.ndarray:
	"""Shaped reward for the vector run: +0.1 per free step, -1 for pushing a wall."""
	commanded = acts[:, 0]
	achieved = obs["linear_vel"][:, 0]
	blocked = (commanded > 0.01) & (np.abs(achieved) < 0.01)
	return np.where(blocked, -1.0, 0.1).astype(np.float32)


def sim_config(args) -> dict:
	"""Headless + fast config shared by every worker (and the sequential run)."""
	cam_w, cam_h = (int(v) for v in args.camera_size.lower().split("x"))
	step_m = 1.0 / args.steps_per_meter
	fixed_dt = step_m / args.speed
	return dict(
		verbose=False,
		target_fps=0,
		show_window=True,
		render_gui=False,
		keyboard_enabled=False,
		quit_key_enabled=False,
		camera_width=cam_w,
		camera_height=cam_h,
		fast_collisions=not args.mesh_collisions,
		max_linear_speed=args.speed,
		max_angular_speed=args.turn_rate,
		fixed_dt=fixed_dt,
		**({"lidar_rays": args.lidar_rays} if args.lidar_rays else {}),
	), fixed_dt


def drive_parallel(names, args) -> tuple:
	"""Drive every map in its own process through VecEnv; returns (results, wall)."""
	cfg, fixed_dt = sim_config(args)

	states = []
	for name in names:
		m = load_map(os.path.join(MAPS_DIR, name))
		st = new_state(ReferencePath(m.poses), name, args)
		st["pending"] = [d for d, _ in m.frames] if args.capture else []
		states.append(st)

	env = None
	try:
		env = VecEnv(len(states), config=cfg, start_method=args.start_method)
		env.reward_fn = reward_fn
		obs = env.reset(None, [st["start"] for st in states])

		wall = 0.0
		t0 = time.perf_counter()
		while any(not st["done"] for st in states):
			actions = np.zeros((len(states), 6), dtype=np.float32)
			for i, st in enumerate(states):
				if not st["done"]:
					actions[i] = plan_action(st, obs["position"][i], obs["yaw"][i], args)
			obs, rew, term, trunc, _ = env.step(actions)

			for i, st in enumerate(states):
				if st["done"]:
					continue
				st["steps"] += 1
				st["rew"] += rew[i]

				v = actions[i, 0]
				blocked = v > 0.0 and abs(obs["linear_vel"][i, 0]) < 1e-6
				st["stalled"] = st["stalled"] + 1 if blocked else 0

				if obs["camera_front"] is not None:
					while st["pending"] and st["s"] >= st["pending"][0]:
						d0 = st["pending"].pop(0)
						st["captured"].append((d0, obs["camera_front"][i].copy()))

				if (st["s"] >= st["length"] - args.goal_tolerance
						or st["steps"] >= st["max_steps"]
						or not bool(obs["running"][i])):
					st["done"] = True
		wall = time.perf_counter() - t0
	finally:
		if env is not None:
			env.close()

	results = []
	for st in states:
		results.append({
			"map": st["map"],
			"length": st["length"],
			"reached": st["s"],
			"steps": st["steps"],
			"wall": wall,
			"xte": np.array(st["xte"]) if st["xte"] else np.zeros(1),
			"sim_time": st["steps"] * fixed_dt,
			"rew_mean": st["rew"] / max(st["steps"], 1),
			"captured": st["captured"],
		})
	return results, wall


def drive_sequential(names, args) -> tuple:
	"""Drive the same maps one after another in this process; returns (results, wall)."""
	cfg, fixed_dt = sim_config(args)
	voxel_sim.Configure(**cfg)
	voxel_sim.Init()

	results = []
	t_wall = 0.0
	try:
		for name in names:
			m = load_map(os.path.join(MAPS_DIR, name))
			path = ReferencePath(m.poses)
			st = new_state(path, name, args)

			obs = voxel_sim.Reset(None,
								  (float(path.xy[0, 0]), float(path.xy[0, 1]),
								   math.degrees(path.start_yaw)))

			t0 = time.perf_counter()
			rew_sum = 0.0
			while (st["s"] < st["length"] - args.goal_tolerance
				   and st["steps"] < st["max_steps"] and obs["running"]):
				obs = voxel_sim.Step(plan_action(st, obs["position"], obs["yaw"], args))
				st["steps"] += 1
				v = st["cmd_forward"]
				blocked = v > 0.0 and abs(obs["linear_vel"][0]) < 1e-6
				st["stalled"] = st["stalled"] + 1 if blocked else 0
				rew_sum += -1.0 if blocked else 0.1
			wall = time.perf_counter() - t0
			t_wall += wall

			results.append({
				"map": name,
				"length": st["length"],
				"reached": st["s"],
				"steps": st["steps"],
				"wall": wall,
				"xte": np.array(st["xte"]) if st["xte"] else np.zeros(1),
				"sim_time": st["steps"] * fixed_dt,
				"rew_mean": rew_sum / max(st["steps"], 1),
				"captured": [],
			})
	finally:
		voxel_sim.Close()

	return results, t_wall


def write_captures(results, out_dir: str) -> int:
	"""Save the captured repeat frames under out_dir/<map>/<d>.jpg (BGR, as observed)."""
	import cv2
	n = 0
	for r in results:
		mdir = os.path.join(out_dir, r["map"])
		os.makedirs(mdir, exist_ok=True)
		for d, img in r["captured"]:
			cv2.imwrite(os.path.join(mdir, f"{d:.4f}.jpg"), img)
			n += 1
	return n


def print_table(results, mode: str, header_footer: bool = True) -> None:
	if header_footer:
		print(f"{'map':<16} {'taught':>8} {'driven':>8} {'steps':>7} {'steps/s':>9} "
			  f"{'wall':>8} {'xte mean':>9} {'xte max':>8} {'rew':>7}")
		print("-" * 78)
	for r in results:
		print(f"{'[' + mode + '] ' + r['map']:<18} {r['length']:7.1f}m {r['reached']:7.1f}m "
			  f"{r['steps']:7d} {r['steps']/max(r['wall'], 1e-9):9.1f} {r['wall']:7.3f}s "
			  f"{r['xte'].mean():8.3f}m {r['xte'].max():7.3f}m {r['rew_mean']:7.3f}")


def main():
	available = sorted(d for d in os.listdir(MAPS_DIR)
					   if os.path.isdir(os.path.join(MAPS_DIR, d)))

	ap = argparse.ArgumentParser(description=__doc__,
								 formatter_class=argparse.RawDescriptionHelpFormatter)
	ap.add_argument("--map", default="all", help=f"map to repeat, or 'all' ({', '.join(available)})")
	ap.add_argument("--steps-per-meter", type=float, default=5.0,
					help="sampling density along the path; this is a floor (default: 5)")
	ap.add_argument("--speed", type=float, default=3.0,
					help="commanded forward speed in m/s (default: 3.0, the taught maximum)")
	ap.add_argument("--turn-rate", type=float, default=1.2,
					help="yaw rate ceiling in rad/s (default: 1.2, the taught maximum)")
	ap.add_argument("--turn-gain", type=float, default=2.5, help="pure pursuit heading gain")
	ap.add_argument("--lookahead", type=float, default=1.5, help="pure pursuit lookahead, m")
	ap.add_argument("--slow-angle", type=float, default=1.2,
					help="heading error (rad) at which the forward command reaches zero")
	ap.add_argument("--min-speed", type=float, default=0.3, help="floor on forward speed, m/s")
	ap.add_argument("--goal-tolerance", type=float, default=0.5,
					help="stop this far from the end of the taught path, m")
	ap.add_argument("--step-budget", type=float, default=3.0,
					help="give up after this multiple of the ideal step count")
	ap.add_argument("--stall-steps", type=int, default=5, help="steps against a wall before backing off")
	ap.add_argument("--mesh-collisions", action="store_true",
					help="test the robot footprint against chunk meshes rather than the "
						 "occupancy grid (mesh-accurate around sub-cell objects, slower)")
	ap.add_argument("--camera-size", default="512x320",
					help="robot camera resolution WxH (default: 512x320)")
	ap.add_argument("--lidar-rays", type=int, default=None,
					help="number of lidar rays in the observation (default: 180)")
	ap.add_argument("--start-method", default="fork", choices=["fork", "spawn"],
					help="multiprocessing start method for the workers (default: fork)")
	ap.add_argument("--capture", metavar="DIR", nargs="?", const="repeat_frames", default=None,
					help="save a repeat frame at each teach frame's distance into DIR")
	ap.add_argument("--no-compare", action="store_true",
					help="skip the single-process comparison run after the vector run")
	ap.add_argument("--tsv", action="store_true",
					help="emit one tab-separated row per map instead of the table")
	args = ap.parse_args()

	if args.map != "all" and args.map not in available:
		ap.error(f"unknown map {args.map!r}; available: {', '.join(available)}")
	names = available if args.map == "all" else [args.map]

	# --- parallel run: every map in its own simulator process -----------------
	print("driving %d map(s) in parallel (one simulator process each, %s)..." %
		  (len(names), args.start_method))
	vec_results, vec_wall = drive_parallel(names, args)

	if args.tsv:
		for r in vec_results:
			print(f"{r['map']}\t{r['length']:.4f}\t{r['reached']:.4f}\t{r['steps']}\t"
				  f"{r['wall']:.6f}\t{r['steps']/max(r['wall'], 1e-9):.4f}\t"
				  f"{r['xte'].mean():.4f}\t{r['xte'].max():.4f}")
	else:
		print()
		print_table(vec_results, "vec")

	# --- single-process comparison (after VecEnv is closed - fork rule) -------
	if not args.no_compare:
		seq_results, seq_wall = drive_sequential(names, args)
		if not args.tsv:
			print_table(seq_results, "seq", header_footer=False)
			print("-" * 78)
			speedup = seq_wall / max(vec_wall, 1e-9)
			print(f"{'total':<16} {sum(r['length'] for r in vec_results):7.1f}m "
				  f"{sum(r['reached'] for r in vec_results):7.1f}m "
				  f"{sum(r['steps'] for r in vec_results):7d} {'':>9} "
				  f"{vec_wall:7.3f}s {'':>18}")
			print(f"\nparallel wall {vec_wall:.3f}s vs sequential wall {seq_wall:.3f}s -> "
				  f"{speedup:.2f}x"
				  + (" faster" if speedup >= 1.0 else " (slower)")
				  + f" (upper bound with perfect scaling: {len(names)}x)")

	if args.capture:
		n = write_captures(vec_results, args.capture)
		print(f"wrote {n} repeat frames under {args.capture}/")


if __name__ == "__main__":
	main()
