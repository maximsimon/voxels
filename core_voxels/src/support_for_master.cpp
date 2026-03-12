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

void movePlayer(VoxelWorld *vw, int curr_chunk, Action *action) {
	Camera camera_old = vw->player_camera;

	Vector3 forward = getForwardDirection(vw->player_camera);
	//Vector3 right = getRightDirection(camera_old);
	//Vector3 left = getLeftDirection(camera_old);
	//Vector3 back = getBackDirection(camera_old);
	
	// move
	if (action->linear_vel.x > 0) {
		if (!CheckCollision(&vw->player_camera, vw->main_map.chunks[curr_chunk], 0.1f, forward, &vw->maze_mesh)) {
			CameraMove(&vw->player_camera, forward);
		}
	}

	// rotate
	//if (IsKeyDown(KEY_W)) {
	//	CameraRotate(camera, UP);
	//}
	//if (IsKeyDown(KEY_S)) {
	//	CameraRotate(camera, DOWN);
	//}
	//if (IsKeyDown(KEY_A)) {
	//	CameraRotate(camera, RIGHT);
	//}
	//if (IsKeyDown(KEY_D)) {
	//	CameraRotate(camera, LEFT);
	//}
}

void checkControls(VoxelWorld *vw, int curr_chunk) {
	if (IsKeyPressed(KEY_P)) {
		vw->player_mode = !vw->player_mode;
	}
	if (IsKeyPressed(KEY_V)) {
		vw->player_view = !vw->player_view;
		vw->player_angle = getPlayerAngle(vw->player_camera);
	}

	// control player_camera and Movement1person
	if (vw->player_mode && vw->player_view) {
		CheckMovement1person(&vw->player_camera, vw->main_map.chunks[curr_chunk], &vw->maze_mesh);
		vw->current_camera = vw->player_camera;
		vw->player_angle = getPlayerAngle(vw->player_camera);
		
	// control edit_camera but Movement1person
	} else if(vw->player_mode) {
		CheckMovement1person(&vw->player_camera, vw->main_map.chunks[curr_chunk], &vw->maze_mesh);
		vw->current_camera = vw->edit_camera;
		vw->player_angle = getPlayerAngle(vw->player_camera);
	// control edit_camera and MovementEdit
	} else {
		CheckMovementEdit(&vw->edit_camera, vw->main_map.chunks[curr_chunk]);
		vw->current_camera = vw->edit_camera;
	}

}


