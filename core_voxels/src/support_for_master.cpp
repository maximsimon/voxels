// UTILS FOR MASTER_VOXEL.C - check keys pressed, handle action (simulation input) 

#include "raylib.h"
#include "raymath.h"
#include <stdio.h>
#include <stdlib.h>
#include <sstream>
#include <cstring>
#include "player_movement.hpp"
#include "camera_movement.hpp"
#include "collisions.hpp"
#include "small_handy_stuff.hpp"
#include "map.hpp"
#include "master_voxel.hpp"
#include "support_for_master.hpp"
#include "data_types.hpp"

void movePlayerWithAction(VoxelWorld *vw, int curr_chunk, Action *action) {
	Vector3 forward = getForwardDirection(vw->player_camera);
	Vector3 right = getRightDirection(vw->player_camera);
	Vector3 left = getLeftDirection(vw->player_camera);
	Vector3 back = getBackDirection(vw->player_camera);
	
	// move
	if (!CheckCollision(&vw->player_camera, vw->main_map.chunks[curr_chunk], 0.1f, forward, &vw->maze_mesh)) {
		CameraMove(&vw->player_camera, forward, action->linear_vel.x);
	}
	if (!CheckCollision(&vw->player_camera, vw->main_map.chunks[curr_chunk], 0.1f, right, &vw->maze_mesh)) {
		CameraMove(&vw->player_camera, right, action->linear_vel.y);
	}
	//rotate
	CameraRotate(&vw->player_camera, RIGHT, action->angular_vel.y);
	vw->player_angle = getPlayerAngle(vw->player_camera);
}

void handleActionsAndKeys(VoxelWorld *vw, int curr_chunk, Action *action) {
	Camera camera_old = vw->player_camera;

	Vector3 forward = getForwardDirection(vw->player_camera);
	Vector3 right = getRightDirection(vw->player_camera);
	Vector3 left = getLeftDirection(vw->player_camera);
	Vector3 back = getBackDirection(vw->player_camera);
	
	bool player_moved_by_keys = false;

	player_moved_by_keys = checkControls(vw, curr_chunk);	
	
	if (player_moved_by_keys == false) {
		movePlayerWithAction(vw, curr_chunk, action);
	}

}

bool checkControls(VoxelWorld *vw, int curr_chunk) {
	bool player_moved_by_keys = false;
	if (IsKeyPressed(KEY_P)) {
		vw->player_mode = !vw->player_mode;
	}
	if (IsKeyPressed(KEY_V)) {
		vw->player_view = !vw->player_view;
		vw->player_angle = getPlayerAngle(vw->player_camera);
	}
	// open input textbox for teleport goal coordinations
	if (IsKeyPressed(KEY_T)) {
		// reset teleport_text values
		vw->teleport_text.text_active = true;
		vw->teleport_text.letter_count = 0;
		for (int i = 0; i < vw->teleport_text.MAX_INPUT_CHARS; i++) vw->teleport_text.text[i] = '\0';
		//memset(vw->teleport_text.text, 0, sizeof(vw->teleport_text.text);      // NOTE: One extra space required for null terminator char '\0'
		
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
		
		Vector3 goal_pose = { 0 };
		std::stringstream ss(vw->teleport_text.text);
		ss >> goal_pose.x >> goal_pose.y >> goal_pose.z;
		teleport(&vw->player_camera, goal_pose);
	}	

	// control player_camera and Movement1person
	if (vw->player_mode && vw->player_view) {
		player_moved_by_keys = CheckMovement1person(&vw->player_camera, vw->main_map.chunks[curr_chunk], &vw->maze_mesh);
		vw->current_camera = vw->player_camera;
		vw->player_angle = getPlayerAngle(vw->player_camera);
		
	// control edit_camera but Movement1person
	} else if(vw->player_mode) {
		player_moved_by_keys = CheckMovement1person(&vw->player_camera, vw->main_map.chunks[curr_chunk], &vw->maze_mesh);
		vw->current_camera = vw->edit_camera;
		vw->player_angle = getPlayerAngle(vw->player_camera);
	// control edit_camera and MovementEdit
	} else {
		CheckMovementEdit(&vw->edit_camera, vw->main_map.chunks[curr_chunk]);
		vw->current_camera = vw->edit_camera;
	}
	
	return player_moved_by_keys;

}


