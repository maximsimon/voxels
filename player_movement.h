// PLAYER (1st person camera) MOVEMENT (translation, rotation)

#ifndef PLAYER_MOVEMENT_H
#define PLAYER_MOVEMENT_H

#include "raylib.h"
#include "raymath.h"
#include <stdio.h>
#include "map.h"

float SPEED = 0.1f;	
float TURN_SPEED = 0.05f;
int SCREENSHOT_COUNTER = 0;

enum HEADING {
	RIGHT,
	LEFT,
	UP,
	DOWN
};


void CheckMovement1person(Camera *camera, chunkMap map);		// Check for keyboard keys that move player
void CameraMove(Camera *camera, Vector3 direction);		// Translate camera
void CameraRotate(Camera *camera, int HEADING);			// Rotate camera
bool CheckCollision(Vector3 pose, Vector3 direction, chunkMap map);		// Check if player is colliding with any voxel

// Check for keyboard keys that move player
void CheckMovement1person(Camera *camera, chunkMap map) {
	
	Vector3 forward = Vector3Subtract(camera->target, camera->position);
	Vector3 right = Vector3CrossProduct(forward, camera->up);
	
	Camera camera_old = *camera;
	
	// move
	if (IsKeyDown(KEY_UP)) {
		if (!CheckCollision(camera->position, forward, map)) {
			CameraMove(camera, forward);
		}
	}
	if (IsKeyDown(KEY_DOWN)) {
		if (!CheckCollision(camera->position, Vector3Negate(forward), map)) {
			CameraMove(camera, Vector3Negate(forward));
		}
	}
	if (IsKeyDown(KEY_RIGHT)) {
		if (!CheckCollision(camera->position, right, map)) {
			CameraMove(camera, right);
		}
	}
	if (IsKeyDown(KEY_LEFT)) {
		if (!CheckCollision(camera->position, Vector3Negate(right), map)) {
			CameraMove(camera, Vector3Negate(right));
		}
	}

	// rotate
	if (IsKeyDown(KEY_W)) {
		CameraRotate(camera, UP);
	}
	if (IsKeyDown(KEY_S)) {
		CameraRotate(camera, DOWN);
	}
	if (IsKeyDown(KEY_A)) {
		CameraRotate(camera, RIGHT);
	}
	if (IsKeyDown(KEY_D)) {
		CameraRotate(camera, LEFT);
	}

	// take pick
	char img_fname[64];
	sprintf(img_fname, "camera_view_%d.png", SCREENSHOT_COUNTER);
	if (IsKeyPressed(KEY_SPACE)) {
		TakeScreenshot(img_fname); 	
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
		targetPosition = Vector3RotateByAxisAngle(targetPosition, camera->up, angle * TURN_SPEED);
	 
	} else if (up_not_right == true) {				// right or left
		Vector3 forward = Vector3Subtract(camera->target, camera->position);
		Vector3 right = Vector3CrossProduct(forward, camera->up);
		targetPosition = Vector3RotateByAxisAngle(targetPosition, right, angle * TURN_SPEED);
	}

	camera->target = Vector3Add(camera->position, targetPosition);
}

// NEXTSTEP nextstep todo TODO
// Check if player is colliding with any voxel
bool CheckCollision(Vector3 pose, Vector3 direction, chunkMap map) {
	bool collision = false;
	float padding = 1.0f;
	// z pozice ziskat pozici v mape, je tam 1?

	// find in which voxel player is going to be located
	direction.y = 0;
	direction = Vector3Normalize(direction);

    	direction = Vector3Scale(direction, SPEED);
	
	Vector3 future_pose = Vector3Add(pose, direction);

	int map_x = 0;
	int map_z = 0; 
	float body_padding_x[3] = {0.0, 0.5, -0.5};
	// check if that voxel is air or matter
	for (int i = 0; i < 3; i++) {	
		for (int j = 0; j < 3; j++) {
			map_x = (int)(future_pose.x + body_padding_x[i]);
			map_z = (int)(future_pose.z + body_padding_x[3-j]);
			if ((map_x < 16) && (map_z < 16)) {
				if (map.map[map_x + map_z * 16] == 1) collision = true;
				break;
			}
		}
	}
	return collision;

}

#endif

