# play.py - launch the simulation in a window and drive the robot with the keyboard.
# Every pose the robot visits while the run lasts is recorded to a CSV trail file, so
# the path can be replayed onto the map image afterwards.
#
# Usage:
#   python scripts/play.py [--world NAME] [--spawn-x X] [--spawn-z Z] [--outdir DIR]
#
# This is the Python equivalent of ./build/master_main: the window is shown and the
# C++ simulation core handles the keyboard, so the script only needs to keep calling
# Step() and let the key presses move the robot.
#
# Controls (identical to ./build/master_main):
#   P          enter player mode (drive the robot)
#   O          leave player mode (back to the god camera)
#   V          toggle first-person robot view
#   Arrows     forward/back/strafe the robot in player mode (god camera in edit mode)
#   A / D      rotate the robot left/right
#   Space      screenshot (camera_view_N.png)
#   T          type a teleport goal "x z yaw_deg", Enter to confirm, Backspace to edit
#   R          reload the current world definition live
#   Q or window close   quit

import sys
sys.path.insert(0, "build")

import argparse
import csv
import math
import os
import time

import numpy as np
import voxel_sim as v


def _safe_name(name):
	return "".join(c if (c.isalnum() or c in "-_") else "_" for c in name)

def _open_trail(outdir, world_name, map_path, colors):
	os.makedirs(outdir, exist_ok=True)
	stamp = time.strftime("%Y%m%d_%H%M%S")
	path = os.path.join(outdir, "trail_%s_%s.csv" % (_safe_name(world_name), stamp))
	f = open(path, "w", newline="")
	f.write("# world=%s\n" % world_name)
	f.write("# map=%s\n" % map_path)
	f.write("# colors=%s\n" % colors)
	f.write("# recorded=%s\n" % time.strftime("%Y-%m-%d %H:%M:%S"))
	writer = csv.writer(f)
	writer.writerow(["step", "x", "y", "z", "yaw_deg"])
	f.flush()
	return path, f, writer


def _world_meta():
	info = v.WorldInfo()
	return info["name"], info["map"], ",".join(info["colors"])


def main():
	ap = argparse.ArgumentParser()
	ap.add_argument("--world", default=None, help="reset into a registered world; defaults to the one loaded from worlds.config")
	ap.add_argument("--spawn-x", type=float, default=None, help="spawn x in map cells (open cell, a few pixels from any wall)")
	ap.add_argument("--spawn-z", type=float, default=None, help="spawn z in map cells (open cell, a few pixels from any wall)")
	ap.add_argument("--outdir", default="core_voxels/resources/playback", help="directory for the trail CSV files")
	args = ap.parse_args()

	print("registered worlds:", v.WORLDS)

	v.Configure(
		show_window=True,
		render_gui=True,
		keyboard_enabled=True,
		quit_key_enabled=True,
	)

	obs = v.Init()
	if not obs["running"]:
		return

	if args.world is not None or (args.spawn_x is not None and args.spawn_z is not None):
		pos = (args.spawn_x, args.spawn_z, 0.0) if args.spawn_x is not None and args.spawn_z is not None else None
		obs = v.Reset(args.world, pos)

	print("P to drive the robot (arrows + A/D), V for first person, T to teleport, R reload, Q to quit.")
	print("start pose:", obs["position"])

	trail_world, trail_map, trail_colors = _world_meta()
	trail_path, trail_file, trail_writer = _open_trail(args.outdir, trail_world, trail_map, trail_colors)
	step_count = 0

	def record(obs):
		nonlocal trail_path, trail_file, trail_writer, trail_world, trail_map, trail_colors, step_count
		current_world = v.CurrentWorld()
		if current_world != trail_world:
			trail_file.close()
			print("world switched to '%s' - starting a new trail file" % current_world)
			trail_world, trail_map, trail_colors = _world_meta()
			trail_path, trail_file, trail_writer = _open_trail(args.outdir, trail_world, trail_map, trail_colors)
		pos = obs["position"]
		trail_writer.writerow([step_count, "%.6f" % pos[0], "%.6f" % pos[1], "%.6f" % pos[2], "%.6f" % math.degrees(obs["yaw"])])
		trail_file.flush()

	last_pos = obs["position"].copy()
	record(obs)
	print("recording path to", trail_path)

	try:
		while obs["running"]:
			obs = v.Step(np.zeros(6, dtype=np.float32))
			step_count += 1
			pos = obs["position"]
			if pos[0] != last_pos[0] or pos[1] != last_pos[1] or pos[2] != last_pos[2]:
				record(obs)
				last_pos = pos.copy()
	finally:
		trail_file.close()

	v.Close()
	print("trail saved to", trail_path)
	print("bye")


if __name__ == "__main__":
	main()