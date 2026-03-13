// UTILS FOR MASTER_VOXEL.C 

#include "raylib.h"
#include "raymath.h"
#include <stdio.h>
#include <stdlib.h>
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


