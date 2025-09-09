// MY OWN DEFINITION OF CAMERA TO HAVE MORE DIRECT CONTROL OVER IT (AND ITS BINDINGS)

#ifndef CAMERA_MOVEMENT_H
#define CAMERA_MOVEMENT_H

#include "raylib.h"
#include "raymath.h"
#include <stdio.h>

float SPEED = 0.4f;	
float TURN_SPEED = 0.1f;

enum HEADING {
	RIGHT,
	LEFT,
	UP,
	DOWN
};


void CheckMovement(Camera *camera);
void CameraMove(Camera *camera, Vector3 direction);
void CameraRotate(Camera *camera, int HEADING);

// check for keyboard keys that move player
void CheckMovement(Camera *camera) {
	
	Vector3 forward = Vector3Subtract(camera->target, camera->position);
	Vector3 right = Vector3CrossProduct(forward, camera->up);
	
	// move
	if (IsKeyDown(KEY_UP)) {
		CameraMove(camera, forward);
	}
	if (IsKeyDown(KEY_DOWN)) {
		CameraMove(camera, Vector3Negate(forward));
	}
	if (IsKeyDown(KEY_RIGHT)) {
		CameraMove(camera, right);
	}
	if (IsKeyDown(KEY_LEFT)) {
		CameraMove(camera, Vector3Negate(right));
	}

	// rotate
	if (IsKeyDown(KEY_R)) {
		CameraRotate(camera, RIGHT);
	}
	if (IsKeyDown(KEY_T)) {
		CameraRotate(camera, LEFT);
	}
}

// move camera
void CameraMove(Camera *camera, Vector3 direction) {
	
	direction.y = 0;
	direction = Vector3Normalize(direction);

    	direction = Vector3Scale(direction, SPEED);
	
	camera->position = Vector3Add(camera->position, direction);
	camera->target = Vector3Add(camera->target, direction);
}

// rotate camera
void CameraRotate(Camera *camera, int HEADING) {
	
	int angle = 0;
	if (HEADING == RIGHT) angle = 1;
	if (HEADING == LEFT) angle = -1;
	Vector3 targetPosition = Vector3Subtract(camera->target, camera->position);
	targetPosition = Vector3RotateByAxisAngle(targetPosition, camera->up, angle * TURN_SPEED);
	
	camera->target = Vector3Add(camera->position, targetPosition);
}
#endif

