// MASTER MAIN - starts and steps the Voxel World simulation.
// Initialization happens once via master_init_sim(), then master_step_sim()
// is called in a loop from src/main.cpp to advance a single frame each call.

#include "raylib.h"

#include "master_voxel.hpp"
#include "player_movement.hpp"
#include "master_main.hpp"
#include "config_master.hpp"

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

	//TODO: add some error handling and printing if file does not load
	//TODO temp params here to get it running -> then move to yaml file
	//Image mazemap_image = LoadImage("core_voxels/resources/map_images/map_paper.png");	//TODO: add some error handling and printing if file does not load
	Vector3 player_pose = { -2.0f, 0.5f, -2.0f };
	Vector3 player_direction = { -2.0f, 0.5f, -1.0f };
	int step_size = 2;		// in milliseconds
	
	g_vw_instance = init_sim(player_pose, player_direction, step_size);	// init voxel world simulation
}

// steps one simulation frame; returns false when the window/quit key requests exit
bool master_step_sim() {
	if (WindowShouldClose() || IsKeyPressed(KEY_Q)) {
		CloseWindow();	//TODO: cleaner and move closing of simulation and cleanup to core_voxels + end the program, not just window (in future make simulation restartable from window or code as well)
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

	master_observation.linear_vel.x = g_vw_observation->linear_vel.x;
	master_observation.linear_vel.y = g_vw_observation->linear_vel.y;
	master_observation.linear_vel.z = g_vw_observation->linear_vel.z;
	
	master_observation.angular_vel.x = g_vw_observation->angular_vel.x;
	master_observation.angular_vel.y = g_vw_observation->angular_vel.y;
	master_observation.angular_vel.z = g_vw_observation->angular_vel.z;	

	memcpy(master_observation.lidar_scan, g_vw_observation->lidar_scan, sizeof(master_observation.lidar_scan));

	return true;
}
