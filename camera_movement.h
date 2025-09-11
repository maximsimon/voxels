// CAMERA MOVEMENT (translation, rotation)

#ifndef CAMERA_MOVEMENT_H
#define CAMERA_MOVEMENT_H

#include "raylib.h"
#include "raymath.h"
#include <stdio.h>
#include "map.h"

float SPEED = 0.1f;	
float TURN_SPEED = 0.05f;

enum HEADING {
	RIGHT,
	LEFT,
	UP,
	DOWN
};


void CheckMovement(Camera *camera, chunkMap map);		// Check for keyboard keys that move player
void CameraMove(Camera *camera, Vector3 direction);		// Translate camera
void CameraRotate(Camera *camera, int HEADING);			// Rotate camera
bool CheckCollision(Vector3 pose, chunkMap map);		// Check if player is colliding with any voxel

// Check for keyboard keys that move player
void CheckMovement(Camera *camera, chunkMap map) {
	
	Vector3 forward = Vector3Subtract(camera->target, camera->position);
	Vector3 right = Vector3CrossProduct(forward, camera->up);
	
	Camera camera_old = *camera;
	
	// move
	if (IsKeyDown(KEY_UP)) {
		CameraMove(camera, forward);
		if (CheckCollision(camera->position, map)) {
			*camera = camera_old;		// reset
		} else {
			camera_old = *camera;		// update (in case of collision in other direction)
		}
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

// Translate camera
void CameraMove(Camera *camera, Vector3 direction) {
	
	direction.y = 0;
	direction = Vector3Normalize(direction);

    	direction = Vector3Scale(direction, SPEED);
	
	camera->position = Vector3Add(camera->position, direction);
	camera->target = Vector3Add(camera->target, direction);
}

// Rotate camera
void CameraRotate(Camera *camera, int HEADING) {
	
	int angle = 0;
	if (HEADING == RIGHT) angle = 1;
	if (HEADING == LEFT) angle = -1;
	Vector3 targetPosition = Vector3Subtract(camera->target, camera->position);
	targetPosition = Vector3RotateByAxisAngle(targetPosition, camera->up, angle * TURN_SPEED);
	
	camera->target = Vector3Add(camera->position, targetPosition);
}

// Check if player is colliding with any voxel
bool CheckCollision(Vector3 pose, chunkMap map) {
    bool collision = false;

	// z pozice ziskat pozici v mape, je tam 1?
	
	// find in which voxel player is located
	int map_x = (int)(pose.x + 0.5);
	int map_y = (int)(pose.y + 0.5);
	
	// check if that voxel is air or matter
	if (map.map[map_x + map_y * 16] == 1) collision = true;

	return collision;
}

#endif

