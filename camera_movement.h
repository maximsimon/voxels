// CAMERA (edit mode) MOVEMENT (translation, rotation)

#ifndef CAMERA_MOVEMENT_H
#define CAMERA_MOVEMENT_H

#include "raylib.h"
#include "raymath.h"
#include <stdio.h>
#include "map.h"

float SPEED_EDIT = 0.1f;	
float TURN_SPEED_EDIT = 0.05f;

void CheckMovementEdit(Camera *camera, Camera *player, chunkMap map);		// Check for keyboard keys that move player
void CameraMoveEdit(Camera *camera, Vector3 direction);		// Translate camera
void CameraMoveUpDownEdit(Camera *camera, Vector3 direction);		// Translate camera
void CameraRotateEdit(Camera *camera, int HEADING);			// Rotate camera

// Check for keyboard keys that move player
void CheckMovementEdit(Camera *camera, Camera *player, chunkMap map) {
	
	Vector3 forward = Vector3Subtract(camera->target, camera->position);
	Vector3 right = Vector3CrossProduct(forward, camera->up);
	
	Camera camera_old = *camera;
	
	// move
	if (IsKeyDown(KEY_UP)) {
		CameraMoveEdit(camera, forward);
	}
	if (IsKeyDown(KEY_DOWN)) {
		CameraMoveEdit(camera, Vector3Negate(forward));
	}
	if (IsKeyDown(KEY_RIGHT)) {
		CameraMoveEdit(camera, right);
	}
	if (IsKeyDown(KEY_LEFT)) {
		CameraMoveEdit(camera, Vector3Negate(right));
	}
	if (IsKeyDown(KEY_PAGE_UP)) {
		CameraMoveUpDownEdit(camera, camera->up);
	}
	if (IsKeyDown(KEY_PAGE_DOWN)) {
		CameraMoveUpDownEdit(camera, Vector3Negate(camera->up));
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

    	direction = Vector3Scale(direction, SPEED_EDIT);
	
	camera->position = Vector3Add(camera->position, direction);
	camera->target = Vector3Add(camera->target, direction);
}

// Translate camera
void CameraMoveUpDownEdit(Camera *camera, Vector3 direction) {

	direction.x = 0;
	direction.z = 0;
	
	direction = Vector3Normalize(direction);

    	direction = Vector3Scale(direction, SPEED_EDIT);
	
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
	printf("up not right %d \n", up_not_right);		
	Vector3 targetPosition = Vector3Subtract(camera->target, camera->position);
	if (up_not_right == false) {				// up or down
		targetPosition = Vector3RotateByAxisAngle(targetPosition, camera->up, angle * TURN_SPEED_EDIT);
	 
	} else if (up_not_right == true) {				// right or left
		Vector3 forward = Vector3Subtract(camera->target, camera->position);
		Vector3 right = Vector3CrossProduct(forward, camera->up);
		targetPosition = Vector3RotateByAxisAngle(targetPosition, right, angle * TURN_SPEED_EDIT);
	}

	camera->target = Vector3Add(camera->position, targetPosition);
}

#endif

