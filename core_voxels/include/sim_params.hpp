// RUNTIME-TUNABLE SIMULATION PARAMETERS
//
// config_core.hpp holds the compile-time constants (array capacities, chunk size,
// build-time defaults).  This header holds the knobs that can be changed at run
// time - from C++ or from python via voxel_sim.Configure(...) - to trade visual
// fidelity for raw stepping speed.
//
// The defaults reproduce the historical behaviour exactly (50 Hz cap, full-size
// window and camera, keyboard live), so nothing changes unless a caller opts in.
//
// Fields marked INIT-ONLY are read once inside init_sim(); changing them later has
// no effect until the process is restarted.  All other fields are read every step.

#ifndef SIM_PARAMS_H
#define SIM_PARAMS_H

#include "config_core.hpp"

struct SimParams {
	// ---- timing ---------------------------------------------------------
	// Simulated seconds advanced per step_sim() call.  <= 0 falls back to
	// GetFrameTime(), i.e. real elapsed wall time (the original behaviour).
	// A fixed value decouples the physics from the frame rate, which is what
	// makes an RL rollout reproducible and lets the sim run flat out.
	float fixed_dt = 0.02f;

	// raylib frame-rate cap.  <= 0 removes the cap entirely (run as fast as
	// the machine allows).  The original code hard-coded 50.
	int target_fps = 50;

	// ---- main window (third-person / god view) --------------------------
	bool show_window = true;	// INIT-ONLY. false -> window is created but never mapped
	bool render_gui = true;		// draw the world a second time from current_camera + HUD
	int screen_width = SCREEN_WIDTH;	// INIT-ONLY
	int screen_height = SCREEN_HEIGHT;	// INIT-ONLY

	// ---- robot front camera (the camera_front observation) --------------
	// Rendering the robot POV and reading it back to the CPU is by far the most expensive
	// part of a step - about 3 ns per pixel, so resolution is the knob that matters. The
	// camera is always on: an observation without an image is not one this simulator is
	// for. Shrink it rather than looking for a way to switch it off.
	int camera_width = SCREEN_WIDTH;	// INIT-ONLY
	int camera_height = SCREEN_HEIGHT;	// INIT-ONLY

	// ---- lidar ----------------------------------------------------------
	bool lidar_enabled = true;
	int lidar_rays = NUM_LIDAR_RAYS;	// clamped to NUM_LIDAR_RAYS (the array capacity)
	float lidar_range = MAX_LIDAR_RANGE;
	bool draw_lidar_rays = true;		// only matters when render_gui is on

	// ---- collisions -----------------------------------------------------
	// The default check ray-casts the robot's footprint against the chunk *meshes*
	// (GetRayCollisionMesh walks every triangle), which costs ~2.9 ms per step in open
	// space - once rendering is cheap it is the dominant per-step cost by an order of
	// magnitude.  fast_collisions instead looks the footprint up in the occupancy grid the
	// world was built from: ~100x cheaper, but a cell counts as fully blocked, so sub-cell
	// objects (a thin fence, a small bush) collide as full 1x1 blocks. Opt in when the
	// coarser footprint is acceptable.
	bool fast_collisions = false;
	float collision_radius = 0.5f;		// half-width of the robot footprint, world units

	// ---- robot motion ---------------------------------------------------
	// Ceilings applied to the commanded action before it is integrated.
	// <= 0 means "no limit" (the original behaviour - the action was trusted as-is).
	float max_linear_speed = 0.0f;		// m/s
	float max_angular_speed = 0.0f;		// rad/s

	// ---- input ----------------------------------------------------------
	// With keyboard_enabled false the raylib key state never touches the robot or
	// the god camera, so a scripted rollout cannot be perturbed by a stray
	// keypress landing in the window.
	bool keyboard_enabled = true;
	bool quit_key_enabled = true;		// Q / window close ends the run

	// ---- logging --------------------------------------------------------
	bool verbose = true;			// raylib INFO spam + world-build chatter
};

extern SimParams sim_params;

// lidar_rays clamped into [1, NUM_LIDAR_RAYS]
int simActiveLidarRays();

// seconds to advance this step: sim_params.fixed_dt, or real frame time when <= 0
float simStepDelta();

#endif
