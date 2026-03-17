// HANDLE ODOMETRY, WHICH IS PART OF OBSERVATION

#include "raylib.h"
#include "rlgl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cmath>
#include "odometry.hpp"
#include "data_types.hpp"
#include "small_handy_stuff.hpp"

void updateOdometry(VoxelWorld *vw, Action *action, Observation *observation) {
	observation->position = vw->player_camera.position;	
	observation->orientation = getQuaternionOrientation(vw);
	observation->linear_vel = action->linear_vel;
	observation->angular_vel = action->angular_vel;
}

Quaternion getQuaternionOrientation(VoxelWorld *vw) {

	//Quaternion orientation = QuaternionFromMatrix(GetCameraMatrix(vw->player_camera)); // <-- converts 3x3 rotation matrix to quaternion
	
	// Get the view matrix (camera to world)
	Matrix view = MatrixLookAt(vw->player_camera.position, vw->player_camera.target, vw->player_camera.up);
	
	// Invert the view matrix to get world->camera transform
	Matrix cam_world = MatrixInvert(view);
	
	// Extract rotation quaternion
	Quaternion orientation = QuaternionFromMatrix(cam_world);	

	// Normalize
	float norm = sqrt(orientation.x*orientation.x + orientation.y*orientation.y + orientation.z*orientation.z + orientation.w*orientation.w);
	orientation.x /= norm;
	orientation.y /= norm;
	orientation.z /= norm;
	orientation.w /= norm;

	return orientation;
}
