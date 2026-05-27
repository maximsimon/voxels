// UTILS FOR MASTER_VOXEL.C - check keys pressed, handle action (simulation input) 

#include "raylib.h"
#include "raymath.h"
#include <stdio.h>
#include <stdlib.h>
#include <sstream>
#include <cstring>
#include "player_movement.hpp"
#include "god_movement.hpp"
#include "collisions.hpp"
#include "small_handy_stuff.hpp"
#include "master_voxel.hpp"
#include "support_for_master.hpp"
#include "data_types.hpp"

void movePlayerWithAction(VoxelWorld *vw, Action *action, Observation *observation) {
	Vector3 forward = getForwardDirection(vw->player_camera);
	Vector3 right = getRightDirection(vw->player_camera);
	Vector3 left = getLeftDirection(vw->player_camera);
	Vector3 back = getBackDirection(vw->player_camera);

	const float dt = GetFrameTime();		// Treat action->linear_vel and action->angular_vel as proper REP-103 velocities (m/s and rad/s). CameraMove and CameraRotate take a per-frame *displacement*, so we multiply by dt = GetFrameTime() at the call site. Without this scaling the simulator would interpret e.g. linear.x = 1.0 as "move 1 m every frame" instead of "1 m/s".

	// move
	if (!CheckCollision(vw, &vw->player_camera, 0.1f, forward, vw->maze_mesh)) {
		CameraMove(&vw->player_camera, forward, action->linear_vel.x * dt);
		observation->linear_vel.x += action->linear_vel.x;
	}
	if (!CheckCollision(vw, &vw->player_camera, 0.1f, right, vw->maze_mesh)) {
		CameraMove(&vw->player_camera, right, action->linear_vel.z * dt);
		observation->linear_vel.z += action->linear_vel.z;
	}

	//rotate
	CameraRotate(&vw->player_camera, RIGHT, action->angular_vel.y * dt, observation);		// CameraRotate writes the dt-scaled angle into observation->angular_vel.y; overwrite with the raw rate so /cmd_vel_publisher carries rad/s, not per-frame radians.
	observation->angular_vel.y = action->angular_vel.y;
	vw->player_angle = getPlayerAngleDeg(vw->player_camera);
}

void handleActionsAndKeys(VoxelWorld *vw, Action *action, Observation *observation) {
	Camera camera_old = vw->player_camera;

	Vector3 forward = getForwardDirection(vw->player_camera);
	Vector3 right = getRightDirection(vw->player_camera);
	Vector3 left = getLeftDirection(vw->player_camera);
	Vector3 back = getBackDirection(vw->player_camera);
	
	bool player_moved_by_keys = false;
	observation->linear_vel = {0.0, 0.0, 0.0};
	observation->angular_vel = {0.0, 0.0, 0.0};

	player_moved_by_keys = checkControls(vw, action, observation);	
	
	if (player_moved_by_keys == false) {
		movePlayerWithAction(vw, action, observation);
	}

}

bool checkControls(VoxelWorld *vw, Action *action, Observation *observation) {
	bool player_moved_by_keys = false;
	
	if (IsKeyPressed(KEY_P)) {
		vw->player_mode = true;
	}
	if (IsKeyPressed(KEY_O)) {
		vw->player_mode = false;
	}
	if (IsKeyPressed(KEY_V)) {
		vw->player_view = !vw->player_view;
		vw->player_angle = getPlayerAngleDeg(vw->player_camera);
	}
	// open input textbox for teleport goal coordinations
	if (IsKeyPressed(KEY_T)) {
		// reset teleport_text values
		vw->teleport_text.text_active = true;
		vw->teleport_text.letter_count = 0;
		for (int i = 0; i < vw->teleport_text.MAX_INPUT_CHARS; i++) vw->teleport_text.text[i] = '\0';
		teleportGUIinput(vw);
	}
	// if input textbox for teleport opened, keep checking for numbers pressed
	if (vw->teleport_text.text_active == true) {
		teleportGUIinput(vw);
	}
	// confirm teleport goal coordinations
	if (vw->teleport_text.text_active == true && IsKeyPressed(KEY_ENTER)) {
		vw->teleport_text.text[vw->teleport_text.letter_count+1] = '\0'; // Add null terminator at the end of the string
		vw->teleport_text.text_active = false;
		
		// input format: "goal_x goal_z yaw_deg". x and z are the floor-plane coords
		float goal_x = 0.0f, goal_z = 0.0f, yaw_deg = 0.0f;
		std::stringstream ss(vw->teleport_text.text);
		ss >> goal_x >> goal_z;
		bool yaw_provided = static_cast<bool>(ss >> yaw_deg);

		float yaw_rad;
		if (yaw_provided) yaw_rad = yaw_deg * (PI / 180.0f);
		else {
			Vector3 dir = Vector3Subtract(vw->player_camera.target, vw->player_camera.position);
			yaw_rad = atan2f(dir.z, dir.x);
		}
		teleportWithYaw(&vw->player_camera, goal_x, goal_z, yaw_rad);
	}	
	
	// switch camera to proper mode (player vs edit), in PLAYER mode movment is handled from ros_voxels (not here, but trough a ROS topic)
	if (vw->player_mode && vw->player_view)	vw->current_camera = vw->player_camera;
	else if(vw->player_mode) vw->current_camera = vw->edit_camera;
	else {								// control edit_camera and MovementEdit
		CheckMovementEdit(&vw->edit_camera);
		vw->current_camera = vw->edit_camera;
	}
	
	return player_moved_by_keys;

}


