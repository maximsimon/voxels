// UTILS FOR MASTER_VOXEL.C 

#include "raylib.h"
#include "raymath.h"
#include <stdio.h>
#include <stdlib.h>
#include "player_movement.hpp"
#include "camera_movement.hpp"
#include "master_voxel.hpp"
#include "support_for_master.hpp"
#include "data_types.hpp"

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


