// MASTER MAIN - starts, steps and resets the Voxel World simulation.
// Initialization happens once via master_init_sim(), then master_step_sim() is called in a loop from main (cpp or python) to advance a single frame each call.
// master_reset_sim() puts the robot back at a chosen pose, optionally in a different world, without tearing the process down.

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

// The sim works with its own local buffers (vw_action, vw_observation), which are transcribed to/from the external buffers (master_action/master_observation).
// observation and action used by main
Observation master_observation;
Action master_action;	
// observation and action used by simulation core 
static VoxelWorld *vw_instance = nullptr;
static Observation *vw_observation = nullptr;
static Action *vw_action = nullptr;

// pending teleport request, written by external modules and consumed once per simulation frame
struct TeleportRequest {
	float x;
	float z;
	float yaw_rad;
	bool pending;
};
static TeleportRequest master_teleport = { 0.0f, 0.0f, 0.0f, false };

void master_main_teleport(float x, float z, float yaw_rad) {
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

// initializes the entire Voxel World - called once at launch, before the step loop
void master_init_sim() {
	vw_observation = new Observation();
	vw_action = new Action();

	// init voxel world simulation
	vw_instance = init_sim(PLAYER_POSE_INIT, PLAYER_DIRECTION_INIT);	// starting pose comes from config_master.hpp
}

// puts the robot back at (x, z) facing yaw_rad, optionally switching world first; world_name may be NULL or "" to stay in the current world
bool master_reset_sim(const char *world_name, float x, float z, float yaw_rad) {
	if (vw_instance == nullptr) return false;

	if (world_name != nullptr && world_name[0] != '\0') {
		if (!switch_world(vw_instance, world_name)) return false;
	}

	teleportWithYaw(&vw_instance->player_camera, x, z, yaw_rad);
	vw_instance->player_angle = getPlayerAngleDeg(vw_instance->player_camera);

	// drop anything left over from the previous episode so the first step after a reset starts from a clean slate rather than replaying a stale command or teleport
	master_teleport.pending = false;
	master_action.linear_vel = { 0.0f, 0.0f, 0.0f };
	master_action.angular_vel = { 0.0f, 0.0f, 0.0f };
	*vw_action = master_action;
	vw_observation->linear_vel = { 0.0f, 0.0f, 0.0f };
	vw_observation->angular_vel = { 0.0f, 0.0f, 0.0f };

	return true;
}

// fetch current world name (e.g. from external main)
const char *master_current_world_name() {
	if (vw_instance == nullptr) return "";
	return world_name_of(vw_instance->current_world);
}

// steps one simulation frame; returns false when the window/quit key requests exit
bool master_step_sim() {
	if (WindowShouldClose() || (sim_params.quit_key_enabled && sim_params.keyboard_enabled && IsKeyPressed(KEY_Q))) {
		return false;
	}

	// transcribe external action into the sim's local buffer
	vw_action->linear_vel.x = master_action.linear_vel.x;
	vw_action->linear_vel.y = master_action.linear_vel.y;
	vw_action->linear_vel.z = master_action.linear_vel.z;

	vw_action->angular_vel.x = master_action.angular_vel.x;
	vw_action->angular_vel.y = master_action.angular_vel.y;
	vw_action->angular_vel.z = master_action.angular_vel.z;

	// apply any pending external teleport request before stepping the sim, so the new pose is reflected in this frame's observation.
	float tp_x, tp_z, tp_yaw;
	if (master_consume_teleport(&tp_x, &tp_z, &tp_yaw)) {
		teleportWithYaw(&vw_instance->player_camera, tp_x, tp_z, tp_yaw);
	}
	
	// SIMULATION STEP - this is where all calculations of what happens in simulation (i.e. in core_voxels) happens
	step_sim(vw_instance, vw_action, vw_observation);	

	// transcribe sim observation into the external buffer
	vw_observation->camera_front.copyTo(master_observation.camera_front);

	master_observation.position.x = vw_observation->position.x;
	master_observation.position.y = vw_observation->position.y;
	master_observation.position.z = vw_observation->position.z;

	master_observation.orientation.w = vw_observation->orientation.w;
	master_observation.orientation.x = vw_observation->orientation.x;
	master_observation.orientation.y = vw_observation->orientation.y;
	master_observation.orientation.z = vw_observation->orientation.z;
	master_observation.yaw = vw_observation->yaw;

	master_observation.linear_vel.x = vw_observation->linear_vel.x;
	master_observation.linear_vel.y = vw_observation->linear_vel.y;
	master_observation.linear_vel.z = vw_observation->linear_vel.z;

	master_observation.angular_vel.x = vw_observation->angular_vel.x;
	master_observation.angular_vel.y = vw_observation->angular_vel.y;
	master_observation.angular_vel.z = vw_observation->angular_vel.z;

	memcpy(master_observation.lidar_scan, vw_observation->lidar_scan, sizeof(master_observation.lidar_scan));

	return true;
}

// tears the simulation down and releases the world, the render texture and the window
void master_end_sim() {
	if (vw_instance != nullptr) {
		end_sim(vw_instance);
		vw_instance = nullptr;
	} else {
		CloseWindow();
	}
	delete vw_observation;
	delete vw_action;
	vw_observation = nullptr;
	vw_action = nullptr;
}
