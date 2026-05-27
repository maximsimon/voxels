// player (1st person camera) movement (translation, rotation)

#include "raylib.h"
#include "raymath.h"
#include <stdio.h>
#include "collisions.hpp"
#include "small_handy_stuff.hpp"
#include "player_movement.hpp"
#include "data_types.hpp"
#include "config_core.hpp"

int SCREENSHOT_COUNTER = 0;

// teleport whatever camera (player or god) to goal_pose, without changing orientation
void teleport(Camera *camera, Vector3 goal_pose) {
	goal_pose.y = 0.5;		// force y coord to stay the same to keep the simulation 2D for now
	Vector3 camera_direction = Vector3Subtract(camera->target, camera->position);
	camera->position = goal_pose;
	camera->target = Vector3Add(camera->position, camera_direction);
}

// Teleport to floor (x, z) and set heading. yaw_rad is angle in the X-Z plane
// measured from +X (matches getPlayerAngle's atan2(z, x) convention).
void teleportWithYaw(Camera *camera, float x, float z, float yaw_rad) {
	camera->position = (Vector3){ x, 0.5f, z };
	camera->target = (Vector3){ x + cosf(yaw_rad), 0.5f, z + sinf(yaw_rad) };
	camera->up = (Vector3){ 0.0f, 1.0f, 0.0f };
}

// teleport camera based on in simulation input (press T and enter goal pose into textboxt)
void teleportGUIinput(VoxelWorld *vw) {
	 // Get char pressed (unicode character) on the queue
	int key = GetCharPressed();
	
	// Check if more characters have been pressed on the same frame
	while (key > 0) {
		// NOTE: Only allow keys for number (48 - 57), decimal point (46), space bar (32), minus sign (45)
		if ( ( ((key >= 48) && (key <= 57)) || (key == 32) || (key == 46) || (key == 45)) && (vw->teleport_text.letter_count < vw->teleport_text.MAX_INPUT_CHARS)) {
			vw->teleport_text.text[vw->teleport_text.letter_count] = (char)key;
			vw->teleport_text.letter_count++;
		}
		key = GetCharPressed();  // Check next character in the queue
	}

	if (IsKeyPressed(KEY_BACKSPACE)) {
		vw->teleport_text.letter_count--;
		if (vw->teleport_text.letter_count < 0) vw->teleport_text.letter_count = 0;
		vw->teleport_text.text[vw->teleport_text.letter_count] = '\0';
	}
}


// Check for keyboard keys that move player
bool CheckMovement1person(VoxelWorld *vw, Camera *camera, Mesh *mesh, Observation *observation) {
	bool player_moved_by_keys = false;

	Vector3 forward = getForwardDirection(*camera);
	Vector3 right = getRightDirection(*camera);
	Vector3 left = getLeftDirection(*camera);
	Vector3 back = getBackDirection(*camera);
	
	Camera camera_old = *camera;
	
	// move
	if (IsKeyDown(KEY_UP)) {
		if (!CheckCollision(vw, camera, SPEED, forward, mesh)) {
			CameraMove(camera, forward, SPEED);
			player_moved_by_keys = true;
			observation->linear_vel.x += SPEED;
		}
	}
	if (IsKeyDown(KEY_DOWN)) {
		if (!CheckCollision(vw, camera, SPEED, back, mesh)) {
			CameraMove(camera, back, SPEED);
			player_moved_by_keys = true;
			observation->linear_vel.x -= SPEED;
		}
	}
	if (IsKeyDown(KEY_RIGHT)) {
		if (!CheckCollision(vw, camera, SPEED, right, mesh)) {
			CameraMove(camera, right, SPEED);
			player_moved_by_keys = true;
			observation->linear_vel.z += SPEED;
		}
	}
	if (IsKeyDown(KEY_LEFT)) {
		if (!CheckCollision(vw, camera, SPEED, left, mesh)) {
			CameraMove(camera, left, SPEED);
			player_moved_by_keys = true;
			observation->linear_vel.z -= SPEED;
		}
	}

	// rotate - note: observation angular vel is only updated for rotation in 2D (so around y axis) ... the simulation is kept in 2D for now
	/*	FORBIDDING LOOKING UP OR DOWN DURING BEARNAV TESTING SO THAT MATCHING IMAGES IS WITHOUT VERTICAL OFFSET
	if (IsKeyDown(KEY_W)) {
		CameraRotate(camera, UP, TURN_SPEED, observation);
		player_moved_by_keys = true;
		observation->angular_vel.y += TURN_SPEED;
	}
	if (IsKeyDown(KEY_S)) {
		CameraRotate(camera, DOWN, TURN_SPEED, observation);
		player_moved_by_keys = true;
		observation->angular_vel.y -= TURN_SPEED;
	}
	*/
	if (IsKeyDown(KEY_A)) {
		CameraRotate(camera, RIGHT, TURN_SPEED, observation);
		player_moved_by_keys = true;
	}
	if (IsKeyDown(KEY_D)) {
		CameraRotate(camera, LEFT, TURN_SPEED, observation);
		player_moved_by_keys = true;
	}

	// take pick
	char img_fname[64];
	sprintf(img_fname, "camera_view_%d.png", SCREENSHOT_COUNTER);
	if (IsKeyPressed(KEY_SPACE)) {
		TakeScreenshot(img_fname); 	
	}

	return player_moved_by_keys;

}

// Translate camera
void CameraMove(Camera *camera, Vector3 direction, float speed) {
	
	direction.y = 0;
	direction = Vector3Normalize(direction);

    	direction = Vector3Scale(direction, speed);
	
	camera->position = Vector3Add(camera->position, direction);
	camera->target = Vector3Add(camera->target, direction);
}

// Rotate camera
void CameraRotate(Camera *camera, int HEADING, float turn_speed, Observation *observation) {
	
	int angle = 0;
	bool up_not_right = true;
	if (HEADING == UP) {
		angle = 1;
		up_not_right = true;
	}
	if (HEADING == DOWN) {
		angle = -1;
		up_not_right = true;
	}
	if (HEADING == RIGHT) {
		angle = 1;
		up_not_right = false;
	}
	if (HEADING == LEFT) {
		angle = -1;
		up_not_right = false;
	}
	Vector3 targetPosition = Vector3Subtract(camera->target, camera->position);
	if (up_not_right == false) {				// up or down
		targetPosition = Vector3RotateByAxisAngle(targetPosition, camera->up, angle * turn_speed);
		observation->angular_vel.y += angle * turn_speed;
	} else if (up_not_right == true) {				// right or left
		Vector3 forward = getForwardDirection(*camera);		//Vector3Subtract(camera->target, camera->position);
		Vector3 right = getRightDirection(*camera);		//Vector3CrossProduct(forward, camera->up);
		targetPosition = Vector3RotateByAxisAngle(targetPosition, right, angle * turn_speed);
	}

	camera->target = Vector3Add(camera->position, targetPosition);
}


