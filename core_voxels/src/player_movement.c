// player (1st person camera) movement (translation, rotation)

#include "raylib.h"
#include "raymath.h"
#include <stdio.h>
#include "map.h"
#include "collisions.h"
#include "small_handy_stuff.h"
#include "player_movement.h"
#include "data_types.h"

float SPEED = 0.1f;	
float TURN_SPEED = 0.08f;
int SCREENSHOT_COUNTER = 0;

// fetch angle of player model for rendering
float getPlayerAngle(Camera camera) {
	Vector3 y_axis = {0.0f, 1.0f, 0.0f};
	Vector3 player_vec = Vector3Subtract(camera.target, camera.position);
	
	float angle = atan2f(player_vec.z, player_vec.x);  // angle from +X axis in radians
	float angleDeg = - (angle * 180.0f / PI) - TURN_SPEED * 300;
	return angleDeg;	
}

// Check for keyboard keys that move player
void CheckMovement1person(Camera *camera, chunkMap map, Mesh *mesh) {

	Vector3 forward = getForwardDirection(*camera);
	Vector3 right = getRightDirection(*camera);
	Vector3 left = getLeftDirection(*camera);
	Vector3 back = getBackDirection(*camera);
	
	Camera camera_old = *camera;
	
	// move
	if (IsKeyDown(KEY_UP)) {
		if (!CheckCollision(camera, map, SPEED, forward, mesh)) {
			CameraMove(camera, forward);
		}
	}
	if (IsKeyDown(KEY_DOWN)) {
		if (!CheckCollision(camera, map, SPEED, back, mesh)) {
			CameraMove(camera, back);
		}
	}
	if (IsKeyDown(KEY_RIGHT)) {
		if (!CheckCollision(camera, map, SPEED, right, mesh)) {
			CameraMove(camera, right);
		}
	}
	if (IsKeyDown(KEY_LEFT)) {
		if (!CheckCollision(camera, map, SPEED, left, mesh)) {
			CameraMove(camera, left);
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
	Vector3 targetPosition = Vector3Subtract(camera->target, camera->position);
	if (up_not_right == false) {				// up or down
		targetPosition = Vector3RotateByAxisAngle(targetPosition, camera->up, angle * TURN_SPEED);
	 
	} else if (up_not_right == true) {				// right or left
		Vector3 forward = getForwardDirection(*camera);		//Vector3Subtract(camera->target, camera->position);
		Vector3 right = getRightDirection(*camera);		//Vector3CrossProduct(forward, camera->up);
		targetPosition = Vector3RotateByAxisAngle(targetPosition, right, angle * TURN_SPEED);
	}

	camera->target = Vector3Add(camera->position, targetPosition);
}


