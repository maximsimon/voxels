#!/usr/bin/env python3
"""Drive the Voxel World simulation from Python via the pybind11 bridge.

observation dict keys:
    camera_front : np.uint8[H, W, 3]  BGR image from the robot front camera
    position     : np.float32[3]      x, y, z
    orientation  : np.float32[4]      w, x, y, z
    linear_vel   : np.float32[3]
    angular_vel  : np.float32[3]
    lidar_scan   : np.float32[60]     ranges in meters
    running      : bool               false once the window/quit key requests exit

action is a 6-float sequence [vx, vy, vz, wx, wy, wz] (m/s and rad/s).
The raylib keyboard (P player mode, W/S/A/D, K/L, V view, Q quit) still works
while Python drives the loop.

Run from the build directory so the module is importable:

    PYTHONPATH=build python3 examples/python_drive.py
"""

import numpy as np
import voxel_sim


def summarize(obs):
    """Print small-memory overview of an observation dict."""
    out = {}
    for k, v in obs.items():
        if isinstance(v, np.ndarray):
            out[k] = (v.shape, str(v.dtype))
        else:
            out[k] = v
    return out


obs = voxel_sim.Init()
print("init obs:", summarize(obs))

step = 0
try:
    while obs["running"] and step < 1000:
        action = np.zeros(6, dtype=np.float32)          # [vx, vy, vz, wx, wy, wz]
        #action[:3] = (3.0, 0.0, 0.0)                  # drive forward at 1 m/s
        #action[4] = 1.0                               # rotate 0.5 rad/s
        obs = voxel_sim.Step(action)
        step += 1
        if step % 100 == 0:
            pos = obs["position"]
            print(f"step {step}: pos=({pos[0]:.2f}, {pos[1]:.2f}, {pos[2]:.2f}) "
                  f"lidar[0]={obs['lidar_scan'][0]:.2f} m running={obs['running']}")
finally:
    voxel_sim.Close()

print(f"done after {step} steps")
