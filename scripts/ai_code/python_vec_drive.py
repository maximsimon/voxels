#!/usr/bin/env python3
"""Parallel RL-style drive loop using VecEnv, with a throughput comparison.

Runs `--num-envs` simulator processes in parallel, drives them with a simple
policy for `--steps` frames and prints:

  * the shape/type of a stacked observation after one reset,
  * aggregated environment steps/s across all N envs (that is what an RL learner
    collects samples with),
  * the same step count through the single-instance voxel_sim API in one process,
    for a wall-clock and throughput comparison.

After the vector run the script closes it and benchmarks the *single* environment
in the parent process.  This ordering matters: VecEnv must be created before any
voxel_sim.Init() in this process (forking a process that already owns a raylib
window would duplicate the GL context into the children).

Run from the repository root:

    PYTHONPATH=build python3 examples/python_vec_drive.py --num-envs 4 --steps 300
"""

from __future__ import annotations

import argparse
import os
import sys
import time

import numpy as np

_HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, _HERE)                       # so `import vec_env` works from anywhere
sys.path.insert(0, os.path.dirname(_HERE))      # repo root: resources resolve from cwd anyway

from vec_env import VecEnv  # noqa: E402
import voxel_sim            # noqa: E402


def parse_camera(s: str) -> tuple:
    w, h = (int(v) for v in s.lower().split("x"))
    return w, h


def fast_config(args) -> dict:
    """Headless settings: no window, no GUI pass, small camera, grid collisions."""
    cam_w, cam_h = parse_camera(args.camera)
    return dict(
        verbose=False,			# show or hide raylib debug messages
        target_fps=0,
        show_window=False,
        render_gui=False,
        keyboard_enabled=False,
        quit_key_enabled=False,
        camera_width=cam_w,
        camera_height=cam_h,
        fast_collisions=True,
        fixed_dt=0.02,
        max_linear_speed=3.0,
        max_angular_speed=1.5,
    )


def random_poses(n_envs: int, seed: int) -> list:
    """Per-env (x, z, yaw_deg) spread out in a 10x10 open area."""
    rng = np.random.default_rng(seed)
    return [(float(rng.uniform(-6.0, 6.0)),
             float(rng.uniform(-6.0, 6.0)),
             float(rng.uniform(0.0, 360.0))) for _ in range(n_envs)]


def policy(obs, n_envs: int, t: int) -> np.ndarray:
    """Go forward at 1.5 m/s; a slow per-env sine sway so the batch spreads out."""
    act = np.zeros((n_envs, 6), dtype=np.float32)
    act[:, 0] = 1.5
    i = np.arange(n_envs)
    act[:, 4] = 0.6 * np.sin(0.2 * i + 0.1 * t).astype(np.float32)
    return act


def reward_fn(obs, acts) -> np.ndarray:
    """Shaped reward: -1 for grinding against a wall (translation zeroed), else +0.1."""
    commanded = acts[:, 0]
    achieved = obs["linear_vel"][:, 0]
    wanted_motion = commanded > 0.01
    blocked = wanted_motion & (np.abs(achieved) < 0.01)
    return np.where(blocked, -1.0, 0.1).astype(np.float32)


def summarize(obs) -> dict:
    """Small-memory overview of a stacked observation dict."""
    out = {}
    for k, v in obs.items():
        if isinstance(v, np.ndarray):
            out[k] = (v.shape, str(v.dtype))
        elif v is None:
            out[k] = "None"
        else:
            out[k] = v
    return out


def bench_vec(env, n_steps: int, world: str, positions: list) -> dict:
    """Step all envs in parallel for n_steps frames; return timing + reward stats."""
    obs = env.reset(world, positions)
    total_rew = np.zeros(env.n_envs, dtype=np.float32)

    # warm-up: world is cached after the first reset, but let page tables / GL settle
    for _ in range(20):
        env.step(policy(obs, env.n_envs, 0))
    obs = env.reset(world, positions)

    t0 = time.perf_counter()
    for t in range(n_steps):
        acts = policy(obs, env.n_envs, t)
        obs, rew, term, trunc, info = env.step(acts)
        total_rew += rew
        # in a hidden-keyboard setup the sim never stops on its own, but keep the
        # episode going anyway in case a window/limit ever ends one
        for i in np.flatnonzero(term | trunc):
            env.reset_at(int(i), world, positions[i])
    wall = time.perf_counter() - t0

    return dict(
        frames=n_steps,
        wall=wall,
        env_steps=env.n_envs * n_steps,
        steps_per_s=env.n_envs * n_steps / wall,
        ms_per_step=wall / n_steps * 1e3,
        rew_mean=float(total_rew.mean()),
    )


def bench_single(cfg: dict, n_steps: int, pose) -> dict:
    """Drive one frame at a time through voxel_sim in this process."""
    voxel_sim.Configure(**cfg)
    voxel_sim.Init()
    obs = voxel_sim.Reset(None, pose)

    for _ in range(20):
        voxel_sim.Step(policy(obs, 1, 0)[0])
    obs = voxel_sim.Reset(None, pose)

    t0 = time.perf_counter()
    for t in range(n_steps):
        obs = voxel_sim.Step(policy(obs, 1, t)[0])
    wall = time.perf_counter() - t0
    voxel_sim.Close()

    return dict(
        frames=n_steps,
        wall=wall,
        env_steps=n_steps,
        steps_per_s=n_steps / wall,
        ms_per_step=wall / n_steps * 1e3,
        rew_mean=float("nan"),
    )


def fmt(r: dict) -> str:
    return (f"{r['env_steps']:>9d} {r['wall']:>8.3f} {r['steps_per_s']:>9.1f} "
            f"{r['ms_per_step']:>7.3f} {r['rew_mean']:>9.3f}")


def main():
    # arguments possible to path when launching, e.g. python python_vec_drive.py --num-envs 4
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--num-envs", type=int, default=4, help="parallel environments (default: 4)")
    ap.add_argument("--steps", type=int, default=300,
                    help="frames per environment (default: 300)")
    ap.add_argument("--camera", default="128x80",
                    help="robot camera resolution WxH (default: 128x80)")
    ap.add_argument("--world", default="ConferenceWorld",
                    help="world to reset into (default: ConferenceWorld)")
    ap.add_argument("--start-method", default="fork", choices=["fork", "spawn"],
                    help="multiprocessing start method for the workers (default: fork)")
    ap.add_argument("--seed", type=int, default=0, help="RNG seed for the start poses")
    args = ap.parse_args()

    if args.num_envs < 1:
        ap.error("--num-envs must be >= 1")

    positions = random_poses(args.num_envs, args.seed)
    cfg = fast_config(args)

    # --- vector run ---------------------------------------------------------
    print(f"spawning {args.num_envs} simulator processes ({args.start_method})...")
    n0 = time.perf_counter()
    with VecEnv(args.num_envs, config=cfg, start_method=args.start_method) as env:	# create env instance and destroy it after this block
        spawn_s = time.perf_counter() - n0
        env.reward_fn = reward_fn        # example shaping: -1 when pushing a wall
        obs = env.reset(args.world, positions)
        print(f"init took {spawn_s:.2f}s; reset obs:")
        for k, v in summarize(obs).items():
            print(f"  {k:<16} {v}")

        vec_r = bench_vec(env, args.steps, args.world, positions)

    # --- single-env run (after VecEnv is closed - ordering matters for fork)  --- 1 run for speed comparison with N parallel runs
    single_r = bench_single(cfg, args.steps, positions[0])

    # --- comparison ---------------------------------------------------------
    header = f"{'mode':<8} {'env-steps':>9} {'wall (s)':>8} {'steps/s':>9} {'ms/step':>7} {'rew mean':>9}"
    print()
    print(header)
    print("-" * len(header))
    print(f"{'vec N=' + str(args.num_envs):<8} {fmt(vec_r)}")
    print(f"{'single':<8} {fmt(single_r)}")
    print("-" * len(header))
    speedup = vec_r["steps_per_s"] / max(single_r["steps_per_s"], 1e-9)
    per_env = vec_r["steps_per_s"] / args.num_envs / max(single_r["steps_per_s"], 1e-9)
    print(f"aggregate throughput speedup: {speedup:.2f}x  (per-env: {per_env:.2f}x of single)")
    print(f"expected ceiling with perfect scaling: {args.num_envs:.0f}x aggregate")


if __name__ == "__main__":
    main()
