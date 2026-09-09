// MASTER MAIN - starts, steps and resets the Voxel World simulation.
// Initialization happens once via master_init_sim(), then master_step_sim()
// is called in a loop from src/main.cpp to advance a single frame each call.
// master_reset_sim() puts the robot back at a chosen pose, optionally in a
// different world, without tearing the process down.

#include "raylib.h"

#include "master_voxel.hpp"
#include "player_movement.hpp"
#include "world_loading.hpp"
#include "world_config.hpp"
#include "master_main.hpp"
#include "config_master.hpp"
#include "sim_params.hpp"
#include "small_handy_stuff.hpp"

#include <opencv2/opencv.hpp>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Observation master_observation;		// public observation buffer - read by external modules
Action master_action;			// public action buffer - written by external modules

// Pending teleport request, written by external modules and consumed once per simulation frame.  `pending` toggles to false once consumed so a single request results in a single teleport.
struct TeleportRequest {
	float x;
	float z;
	float yaw_rad;
	bool pending;
};
static TeleportRequest master_teleport = { 0.0f, 0.0f, 0.0f, false };

void master_ros_teleport(float x, float z, float yaw_rad) {
	master_teleport.x = x;
	master_teleport.z = z;
	master_teleport.yaw_rad = yaw_rad;
	master_teleport.pending = true;
}

bool master_consume_teleport(float *x, float *z, float *yaw_rad) {
	if (!master_teleport.pending) return false;
	*x = master_teleport.x;
	*z = master_teleport.z;
	*yaw_rad = master_teleport.yaw_rad;
	master_teleport.pending = false;
	return true;
}

// sim state shared between master_init_sim() and master_step_sim()
// the sim works with its own local buffers (g_vw_action, g_vw_observation),
// which are transcribed to/from the public master_action/master_observation
// at the edges of master_step_sim()
static VoxelWorld *g_vw_instance = nullptr;
static Observation *g_vw_observation = nullptr;
static Action *g_vw_action = nullptr;

// initializes the entire Voxel World - called once at launch, before the step loop
void master_init_sim() {
	g_vw_observation = new Observation();
	g_vw_action = new Action();

	// starting pose comes from config_master.hpp
	g_vw_instance = init_sim(PLAYER_POSE_INIT, PLAYER_DIRECTION_INIT);	// init voxel world simulation
}

// puts the robot back at (x, z) facing yaw_rad, optionally switching world first.
// world_name may be NULL or "" to stay in the current world; switch_world() rebuilds
// only when the requested world actually differs, so resetting inside one world is cheap.
// Returns false (changing nothing) if world_name is not a known world.
bool master_reset_sim(const char *world_name, float x, float z, float yaw_rad) {
	if (g_vw_instance == nullptr) return false;

	if (world_name != nullptr && world_name[0] != '\0') {
		if (!switch_world(g_vw_instance, world_name)) return false;
	}

	teleportWithYaw(&g_vw_instance->player_camera, x, z, yaw_rad);
	g_vw_instance->player_angle = getPlayerAngleDeg(g_vw_instance->player_camera);

	// drop anything left over from the previous episode so the first step after a reset
	// starts from a clean slate rather than replaying a stale command or teleport
	master_teleport.pending = false;
	master_action.linear_vel = { 0.0f, 0.0f, 0.0f };
	master_action.angular_vel = { 0.0f, 0.0f, 0.0f };
	*g_vw_action = master_action;
	g_vw_observation->linear_vel = { 0.0f, 0.0f, 0.0f };
	g_vw_observation->angular_vel = { 0.0f, 0.0f, 0.0f };

	return true;
}

const char *master_current_world_name() {
	if (g_vw_instance == nullptr) return "";
	return world_name_of(g_vw_instance->current_world);
}

// steps one simulation frame; returns false when the window/quit key requests exit
bool master_step_sim() {
	// Only report that the run is over - the actual teardown belongs to master_end_sim(),
	// so the caller decides when the world and the GL context go away. Closing the window
	// here used to leave every later cleanup call operating on a dead context.
	if (WindowShouldClose() || (sim_params.quit_key_enabled && sim_params.keyboard_enabled && IsKeyPressed(KEY_Q))) {
		return false;
	}

	// transcribe external action into the sim's local buffer
	g_vw_action->linear_vel.x = master_action.linear_vel.x;
	g_vw_action->linear_vel.y = master_action.linear_vel.y;
	g_vw_action->linear_vel.z = master_action.linear_vel.z;

	g_vw_action->angular_vel.x = master_action.angular_vel.x;
	g_vw_action->angular_vel.y = master_action.angular_vel.y;
	g_vw_action->angular_vel.z = master_action.angular_vel.z;

	// Apply any pending external teleport request before stepping the sim, so the new pose is reflected in this frame's observation.
	float tp_x, tp_z, tp_yaw;
	if (master_consume_teleport(&tp_x, &tp_z, &tp_yaw)) {
		teleportWithYaw(&g_vw_instance->player_camera, tp_x, tp_z, tp_yaw);
	}

	// SIMULATION STEP
	step_sim(g_vw_instance, g_vw_action, g_vw_observation);	// SIMULATION STEP - this is where all calculations of what happens in simulation (i.e. in core_voxels) happens

	// transcribe sim observation into the public buffer
	g_vw_observation->camera_front.copyTo(master_observation.camera_front);

	master_observation.position.x = g_vw_observation->position.x;
	master_observation.position.y = g_vw_observation->position.y;
	master_observation.position.z = g_vw_observation->position.z;

	master_observation.orientation.w = g_vw_observation->orientation.w;
	master_observation.orientation.x = g_vw_observation->orientation.x;
	master_observation.orientation.y = g_vw_observation->orientation.y;
	master_observation.orientation.z = g_vw_observation->orientation.z;
	master_observation.yaw = g_vw_observation->yaw;

	master_observation.linear_vel.x = g_vw_observation->linear_vel.x;
	master_observation.linear_vel.y = g_vw_observation->linear_vel.y;
	master_observation.linear_vel.z = g_vw_observation->linear_vel.z;

	master_observation.angular_vel.x = g_vw_observation->angular_vel.x;
	master_observation.angular_vel.y = g_vw_observation->angular_vel.y;
	master_observation.angular_vel.z = g_vw_observation->angular_vel.z;

	memcpy(master_observation.lidar_scan, g_vw_observation->lidar_scan, sizeof(master_observation.lidar_scan));

	return true;
}

// tears the simulation down and releases the world, the render texture and the window
void master_end_sim() {
	if (g_vw_instance != nullptr) {
		end_sim(g_vw_instance);
		g_vw_instance = nullptr;
	} else {
		CloseWindow();
	}
	delete g_vw_observation;
	delete g_vw_action;
	g_vw_observation = nullptr;
	g_vw_action = nullptr;
}
