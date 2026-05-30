// CAMERA (edit mode) MOVEMENT (translation, rotation)

#include "raylib.h"
#include "raymath.h"
#include <stdio.h>
#include "god_movement.hpp"
#include "player_movement.hpp"
#include "small_handy_stuff.hpp"
#include "data_types.hpp"
#include "config_core.hpp"

// Check for keyboard keys that move
void CheckMovementEdit(Camera *camera) {
	
	Vector3 up = getUpDirection();
	Vector3 down = getDownDirection();
	Vector3 forward = getForwardDirection(*camera);
	Vector3 right = getRightDirection(*camera);
	Vector3 left = getLeftDirection(*camera);
	Vector3 back = getBackDirection(*camera);

	Camera camera_old = *camera;
	
	// move
	if (IsKeyDown(KEY_UP)) {
		CameraMoveEdit(camera, forward);
	}
	if (IsKeyDown(KEY_DOWN)) {
		CameraMoveEdit(camera, back);
	}
	if (IsKeyDown(KEY_RIGHT)) {
		CameraMoveEdit(camera, right);
	}
	if (IsKeyDown(KEY_LEFT)) {
		CameraMoveEdit(camera, left);
	}
	if (IsKeyDown(KEY_PAGE_UP)) {
		CameraMoveUpDownEdit(camera, up);
	}
	if (IsKeyDown(KEY_PAGE_DOWN)) {
		CameraMoveUpDownEdit(camera, down);
	}

	// rotate
	if (IsKeyDown(KEY_W)) {
		CameraRotateEdit(camera, UP);
	}
	if (IsKeyDown(KEY_S)) {
		CameraRotateEdit(camera, DOWN);
	}
	if (IsKeyDown(KEY_A)) {
		CameraRotateEdit(camera, RIGHT);
	}
	if (IsKeyDown(KEY_D)) {
		CameraRotateEdit(camera, LEFT);
	}


}

// Translate camera
void CameraMoveEdit(Camera *camera, Vector3 direction) {

	direction.y = 0;

	direction = Vector3Normalize(direction);

    	direction = Vector3Scale(direction, SPEED_GOD);
	
	camera->position = Vector3Add(camera->position, direction);
	camera->target = Vector3Add(camera->target, direction);
}

// Translate camera
void CameraMoveUpDownEdit(Camera *camera, Vector3 direction) {

	direction.x = 0;
	direction.z = 0;
	
	direction = Vector3Normalize(direction);

    	direction = Vector3Scale(direction, SPEED_GOD);
	
	camera->position = Vector3Add(camera->position, direction);
	camera->target = Vector3Add(camera->target, direction);
}

// Rotate camera
void CameraRotateEdit(Camera *camera, int HEADING) {
	
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
		targetPosition = Vector3RotateByAxisAngle(targetPosition, camera->up, angle * TURN_SPEED_GOD);
	 
	} else if (up_not_right == true) {				// right or left
		Vector3 forward = getForwardDirection(*camera);
		Vector3 right = getRightDirection(*camera);
		targetPosition = Vector3RotateByAxisAngle(targetPosition, right, angle * TURN_SPEED_GOD);
	}

	camera->target = Vector3Add(camera->position, targetPosition);
}

