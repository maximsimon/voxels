#!/usr/bin/env python3
"""Informed repeat: drive a taught VTR trajectory in the simulator in minimum wall time.

"Informed" means the repeat is given the map's recorded odometry and steers by it, rather
than trying to re-localize visually.  The taught poses are the reference path; a pure
pursuit controller tracks them while the simulator runs as fast as the machine allows.

Run from the repository root, so the simulator finds its resources:

    PYTHONPATH=build python3 examples/python_informed_repeat.py
    PYTHONPATH=build python3 examples/python_informed_repeat.py --map my_second_map --gui

The maps live in the world selected by core_voxels/resources/worlds/worlds.config, so the
script never switches worlds - it resets inside whichever world is already loaded.

Speed budget
------------
The simulator advances `fixed_dt` seconds of simulated time per Step(), independent of how
long the step actually took, so the sampling density along the path is set by

    metres per step = speed * fixed_dt

and --steps-per-meter picks fixed_dt from the commanded speed to hit the requested density
(default 5 steps/m, the stated minimum).  Because the controller only ever slows down
below `speed`, that density is a worst-case floor: turns are sampled more finely, never
less.  Wall time then falls out of how cheap a step is - which is why the defaults turn
off the third-person pass, hide the window and render the robot camera at a modest
512x320 - camera cost is about 3 ns per pixel, so resolution is the main lever.
"""

from __future__ import annotations

import argparse
import math
import os
import sys
import time

import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from vtr_map import load_map  # noqa: E402

import voxel_sim  # noqa: E402

MAPS_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "maps")

# Sign relating a commanded yaw rate (action[4]) to the heading the simulator reports in
# obs["yaw"].  It is -1, not +1: commanding a positive yaw rate makes obs["yaw"] *decrease*
# by the matching amount.  The magnitude is exact, only the sense is flipped, and the taught
# maps were recorded through the same convention - integrating their recorded twist with
# `yaw -= w*dt` reproduces their recorded pose track to ~0.5 m over 54 m, while `yaw += w*dt`
# drifts by 20 m.  Left as a named constant rather than "fixed" in the simulator because the
# sign is baked into the ROS bridge and into every map already recorded; flip this to +1.0 if
# the simulator's convention is ever changed to match REP-103.
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
        """Arc length of the closest path point, and the distance to it.

        Searched only in a window around `s_hint` so a path that doubles back on itself
        cannot make progress jump to a different pass over the same ground.
        """
        lo = int(np.searchsorted(self.s, s_hint - window))
        hi = int(np.searchsorted(self.s, s_hint + window)) + 1
        lo, hi = max(lo, 0), min(hi, len(self.xy))
        if hi - lo < 2:
            lo, hi = max(hi - 2, 0), min(lo + 2, len(self.xy))
        d = np.hypot(*(self.xy[lo:hi] - p).T)
        k = int(np.argmin(d))
        return float(self.s[lo + k]), float(d[k])


def repeat(map_name: str, args) -> dict:
    """Drive one taught trajectory. Returns a dict of results."""
    m = load_map(os.path.join(MAPS_DIR, map_name))
    path = ReferencePath(m.poses)

    # metres advanced per step, and therefore the simulated timestep
    step_m = 1.0 / args.steps_per_meter
    fixed_dt = step_m / args.speed
    voxel_sim.Configure(fixed_dt=fixed_dt)

    # world=None keeps whatever worlds.config loaded - the maps were taught in it
    obs = voxel_sim.Reset(None, (float(path.xy[0, 0]), float(path.xy[0, 1]),
                                 math.degrees(path.start_yaw)))

    # frames to capture, keyed by the arc length the teach pass recorded them at
    pending_frames = [d for d, _ in m.frames] if args.capture else []
    captured = []

    s = 0.0
    xte = []          # cross-track error against the taught path, per step
    steps = 0
    max_steps = int(args.step_budget * path.length * args.steps_per_meter) + 100
    stalled = 0

    t0 = time.perf_counter()
    while s < path.length - args.goal_tolerance and steps < max_steps and obs["running"]:
        p = np.array([obs["position"][0], obs["position"][2]], dtype=np.float64)
        s, d = path.project(p, s)
        xte.append(d)

        # pure pursuit: aim at the path point one lookahead further along
        target = path.point_at(s + args.lookahead)
        heading_err = wrap_pi(math.atan2(target[1] - p[1], target[0] - p[0]) - obs["yaw"])

        # slow down as the heading error grows, so tight corners are not overshot; the
        # controller never exceeds args.speed, which is what keeps steps-per-meter a floor
        v = args.speed * max(0.0, 1.0 - abs(heading_err) / args.slow_angle)
        v = max(v, args.min_speed)
        w = max(-args.turn_rate, min(args.turn_rate, args.turn_gain * heading_err))

        # A wall zeroes translation but not rotation. Once we have pushed into one for a
        # few steps, drop the forward command so the robot pivots free instead of grinding
        # against the voxel for the rest of the step budget.
        if stalled > args.stall_steps:
            v = 0.0
            w = args.turn_rate if heading_err >= 0.0 else -args.turn_rate

        action = np.zeros(6, dtype=np.float32)
        action[0] = v                       # forward, m/s
        action[4] = YAW_RATE_SIGN * w       # yaw rate, rad/s (see YAW_RATE_SIGN)
        obs = voxel_sim.Step(action)
        steps += 1

        # linear_vel comes back zeroed on the axis the collision check blocked
        blocked = v > 0.0 and abs(obs["linear_vel"][0]) < 1e-6
        stalled = stalled + 1 if blocked else 0

        while pending_frames and s >= pending_frames[0]:
            d0 = pending_frames.pop(0)
            if obs["camera_front"] is not None:
                captured.append((d0, obs["camera_front"].copy()))

    wall = time.perf_counter() - t0

    return {
        "map": map_name,
        "length": path.length,
        "steps": steps,
        "wall": wall,
        "reached": s,
        "xte": np.array(xte) if xte else np.zeros(1),
        "fixed_dt": fixed_dt,
        "sim_time": steps * fixed_dt,
        "captured": captured,
    }


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
                    help="test the robot footprint against the chunk meshes rather than the "
                         "occupancy grid: mesh-accurate around sub-cell objects, but ~5x slower. "
                         "Both give bit-identical runs on the bundled maps, which never touch a wall.")
    ap.add_argument("--gui", action="store_true",
                    help="show the third-person view and throttle to a watchable frame rate "
                         "(for watching, not timing - see --fps)")
    ap.add_argument("--fps", type=int, default=None,
                    help="frame-rate cap; 0 is uncapped (default: 0, or 60 with --gui). A cap "
                         "throttles the run, so reported wall time is no longer a speed measurement.")
    ap.add_argument("--camera-size", default="512x320",
                    help="robot camera resolution WxH (default: 512x320)")
    ap.add_argument("--lidar-rays", type=int, default=None,
                    help="number of lidar rays in the observation (default: the simulator's 180)")
    ap.add_argument("--capture", metavar="DIR", nargs="?", const="repeat_frames", default=None,
                    help="save a repeat frame at each teach frame's distance into DIR")
    ap.add_argument("--tsv", action="store_true",
                    help="emit one tab-separated row per map instead of the table, for scripting: "
                         "map, taught_m, driven_m, steps, wall_s, steps_per_s, xte_mean_m, xte_max_m")
    args = ap.parse_args()

    if args.map != "all" and args.map not in available:
        ap.error(f"unknown map {args.map!r}; available: {', '.join(available)}")
    targets = available if args.map == "all" else [args.map]

    cam_w, cam_h = (int(v) for v in args.camera_size.lower().split("x"))

    # uncapped by default; --gui without an explicit --fps throttles to something watchable
    target_fps = args.fps if args.fps is not None else (60 if args.gui else 0)

    # Everything that makes a step expensive is off by default: no third-person pass, no
    # visible window, a small robot camera. The keyboard is disabled so a keypress landing
    # in the window cannot perturb a measured run.
    voxel_sim.Configure(
        verbose=False,
        target_fps=target_fps,
        render_gui=args.gui,
        show_window=args.gui,
        keyboard_enabled=args.gui,
        camera_width=cam_w,
        camera_height=cam_h,
        **({"lidar_rays": args.lidar_rays} if args.lidar_rays else {}),
        fast_collisions=not args.mesh_collisions,
        max_linear_speed=args.speed,
        max_angular_speed=args.turn_rate,
    )
    voxel_sim.Init()

    results = []
    try:
        for name in targets:
            results.append(repeat(name, args))
    finally:
        voxel_sim.Close()

    if args.tsv:
        for r in results:
            print(f"{r['map']}\t{r['length']:.4f}\t{r['reached']:.4f}\t{r['steps']}\t"
                  f"{r['wall']:.6f}\t{r['steps']/max(r['wall'], 1e-9):.4f}\t"
                  f"{r['xte'].mean():.4f}\t{r['xte'].max():.4f}")
        return

    print()
    print(f"{'map':<16} {'taught':>8} {'driven':>8} {'steps':>7} {'steps/m':>8} "
          f"{'wall':>8} {'steps/s':>9} {'xte mean':>9} {'xte max':>8}")
    print("-" * 92)
    for r in results:
        print(f"{r['map']:<16} {r['length']:7.1f}m {r['reached']:7.1f}m {r['steps']:7d} "
              f"{r['steps']/max(r['reached'], 1e-9):8.2f} {r['wall']:7.3f}s "
              f"{r['steps']/max(r['wall'], 1e-9):9.1f} "
              f"{r['xte'].mean():8.3f}m {r['xte'].max():7.3f}m")

    total_len = sum(r["reached"] for r in results)
    total_wall = sum(r["wall"] for r in results)
    total_sim = sum(r["sim_time"] for r in results)
    print("-" * 92)
    print(f"{'total':<16} {'':>8} {total_len:7.1f}m {sum(r['steps'] for r in results):7d} "
          f"{'':>8} {total_wall:7.3f}s")
    print(f"\n{total_len:.1f} m of taught path driven in {total_wall:.3f} s wall "
          f"({total_sim:.1f} s of simulated time, {total_sim/max(total_wall,1e-9):.0f}x real time)")
    if target_fps:
        print(f"note: throttled to {target_fps} fps, so the wall time above is the cap, "
              f"not how fast the simulator can go")

    if args.capture:
        os.makedirs(args.capture, exist_ok=True)
        import cv2
        n = 0
        for r in results:
            out = os.path.join(args.capture, r["map"])
            os.makedirs(out, exist_ok=True)
            for dist, img in r["captured"]:
                cv2.imwrite(os.path.join(out, f"{dist:.4f}.jpg"), img)
                n += 1
        print(f"wrote {n} repeat frames under {args.capture}/")


if __name__ == "__main__":
    main()
