# example_vecenv_rollout.py - check a generated dataset in the real simulator.
#
# generation already proves a map is self-consistent (see validate.py); this proves the C++
# agrees, which is the part a pixel-level check cannot know: that the world builds, the robot
# spawns alive inside it, the lidar sees the structure, and a world switch costs what we think.
#
#   PYTHONPATH=build python scripts/worldgen/example_vecenv_rollout.py --dataset myds
#   PYTHONPATH=build python scripts/worldgen/example_vecenv_rollout.py --dataset myds --timing
#   PYTHONPATH=build python scripts/worldgen/example_vecenv_rollout.py --dataset myds --mosaic

import sys
from pathlib import Path

sys.path.insert(0, "build")
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "vector_env"))

import argparse
import time

import numpy as np
from vec_env import VecEnv

_REPO_ROOT = Path(__file__).resolve().parents[2]
_GEN_DIR = _REPO_ROOT / "core_voxels" / "resources" / "generated"

# the trainer spawns inside +-spawn_radius of the origin
SPAWN = (0.0, 0.0, 0.0)


def _lidar_hit_rate(obs: dict) -> float:
	"""Fraction of lidar beams that hit something. 0 means the world built no geometry."""
	scan = obs.get("lidar_scan")
	if scan is None:
		return 0.0
	return float(np.count_nonzero(np.asarray(scan))) / max(1, np.asarray(scan).size)


def _camera_signature(obs: dict) -> np.ndarray:
	cam = obs.get("camera_front")
	return np.zeros(0) if cam is None else np.asarray(cam, dtype=np.float32).ravel()


def main():
	ap = argparse.ArgumentParser(description="verify a generated dataset in the simulator")
	ap.add_argument("--dataset", default="wtest", help="dataset folder under resources/generated")
	ap.add_argument("--steps", type=int, default=5, help="steps to drive after each spawn")
	ap.add_argument("--spawn-radius", type=float, default=6.0,
	                help="sample spawn poses inside this radius of the origin, like the trainer")
	ap.add_argument("--mosaic", action="store_true", help="open the single tiled window")
	ap.add_argument("--timing", action="store_true",
	                help="time every world switch (this is what a per-episode reset costs)")
	args = ap.parse_args()

	folder = _GEN_DIR / args.dataset
	if not (folder / f"{args.dataset}.registry").exists():
		raise SystemExit(f"no registry at {folder / (args.dataset + '.registry')} - "
						 f"generate it with: python -m scripts.worldgen.generate_all "
						 f"--dataset {args.dataset}")

	rng = np.random.default_rng(0)
	actions = np.zeros((1, 6), dtype=np.float32)
	actions[0, 0] = 1.5

	# the simulation is never initialized in this process. VecEnv's workers fork and each run
	# their own Configure/Init, and the parent already having a window would leave the
	# inherited state unable to set show_window - the sim reads that flag exactly once, at Init
	with VecEnv(1, show_mosaic=args.mosaic) as env:
		# load the dataset registry directly - no worlds.config edit needed
		pool = tuple(env.load_worlds(str(folder)))
		print(f"dataset '{args.dataset}': {len(pool)} world(s)\n")

		headers = f"{'world':<22} {'fill':>6} {'tier':>7} {'layouts':<20} {'lidar':>6} {'cam mean':>9}"
		print(headers)
		print("-" * len(headers))

		signatures = {}
		problems = []
		timings = []

		for name in pool:
			pose = (float(rng.uniform(-args.spawn_radius, args.spawn_radius)),
					float(rng.uniform(-args.spawn_radius, args.spawn_radius)),
					float(rng.uniform(0, 360)))

			start = time.perf_counter()
			obs = env.reset_at(0, world=name, position=pose)
			switch = time.perf_counter() - start

			if args.timing:
				timings.append((name, switch))

			if not obs.get("running", False):
				problems.append(f"{name}: robot did not spawn alive at {pose}")

			# step returns the gym 5-tuple; reset_at returns one unstacked obs dict
			for _ in range(args.steps):
				obs, _reward, terminated, _trunc, _info = env.step(actions)
				if terminated[0]:
					break
			if not obs.get("running", False):
				problems.append(f"{name}: robot stopped during the rollout")

			hit = _lidar_hit_rate(obs)
			if hit <= 0.0:
				problems.append(f"{name}: lidar sees nothing - the world built no geometry")

			cam = _camera_signature(obs)
			signatures[name] = cam
			meta = _meta_for(name)
			print(f"{name:<22} {meta.get('fill', 0):>6.1%} {meta.get('density_tier', '?'):>7} "
				  f"{'+'.join(meta.get('layouts', [])):<20} {hit:>6.1%} "
				  f"{float(cam.mean()) if cam.size else 0.0:>9.2f}")

		print()
		_report_variety(pool, signatures)
		if args.timing:
			_report_timing(timings)
		if problems:
			print(f"\n{len(problems)} problem(s):")
			for p in problems:
				print(f"  - {p}")
			raise SystemExit(1)
		print(f"\nall {len(pool)} world(s) built, spawned alive and returned lidar")


def _meta_for(name: str) -> dict:
	"""Pull one world's row out of the dataset manifest, if it is importable."""
	try:
		import importlib.util
		folder = _GEN_DIR
		for candidate in folder.glob("*/" + name.rsplit("_", 1)[0] + ".worlds.py"):
			spec = importlib.util.spec_from_file_location("ds_manifest", candidate)
			module = importlib.util.module_from_spec(spec)
			spec.loader.exec_module(module)
			for world in module.WORLDS:
				if world["name"] == name:
					return world
	except Exception as exc:  # a manifest is a convenience here, not a requirement
		print(f"(manifest unavailable: {exc})")
	return {}


def _report_variety(pool, signatures):
	"""Distinct camera signatures are the cheapest proof two worlds really differ."""
	print("pairwise camera difference (mean abs difference, 0-255):")
	sig = [signatures[n] for n in pool if signatures[n].size]
	if len(sig) < 2:
		return
	diffs = []
	for i in range(len(sig)):
		for j in range(i + 1, len(sig)):
			a, b = sig[i], sig[j]
			n = min(a.size, b.size)
			diffs.append(float(np.abs(a[:n] - b[:n]).mean()))
	diffs.sort()
	print(f"  {len(diffs)} pairs, min {diffs[0]:.2f}, median {np.median(diffs):.2f}, "
	      f"max {diffs[-1]:.2f}")
	if diffs[0] < 1.0:
		print("  WARNING: at least two worlds render identically from the same spawn")


def _report_timing(timings):
	print("\nworld switch cost (destroy + rebuild, what every episode end pays):")
	if not timings:
		return
	values = sorted(v for _, v in timings)
	print(f"  min {values[0]:.3f}s  median {np.median(values):.3f}s  max {values[-1]:.3f}s")
	for name, dt in sorted(timings, key=lambda t: -t[1])[:5]:
		print(f"    slowest: {name} {dt:.3f}s")


if __name__ == "__main__":
	main()
