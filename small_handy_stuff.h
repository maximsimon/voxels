// Small handy stuff. Convient functions that shorten the main code by few lines

#ifndef SMALL_HANDY_STUFF_H
#define SMALL_HANDY_STUFF_H

#include "raylib.h"
#include "raymath.h"
#include <stdio.h>

Vector3 getUpDirection();
Vector3 getDownDirection();
Vector3 getRightDirection(Camera camera);
Vector3 getLeftDirection(Camera camera);
Vector3 getBackDirection(Camera camera);
Vector3 getForwardDirection(Camera camera);

Vector3 getUpDirection() {
	Vector3 up = {0.0f, 1.0f, 0.0f};
	return up;
}

Vector3 getDownDirection() {
	Vector3 down = {0.0f, -1.0f, 0.0f};
	return down;
}

Vector3 getRightDirection(Camera camera) {
	Vector3 right = Vector3CrossProduct(getForwardDirection(camera), getUpDirection());
	return right;
}

Vector3 getLeftDirection(Camera camera) {
	Vector3 left = Vector3Negate(getRightDirection(camera));
	return left;
}

Vector3 getBackDirection(Camera camera) {
	Vector3 back = Vector3Negate(getForwardDirection(camera));
	return back;
}

Vector3 getForwardDirection(Camera camera) {
	Vector3 forward = Vector3Subtract(camera.target, camera.position);
	forward.y = 0.0f;
	forward = Vector3Normalize(forward);

	return forward;
}


#endif
