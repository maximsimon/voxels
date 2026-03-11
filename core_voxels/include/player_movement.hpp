// player (1st person camera) movement (translation, rotation)

#ifndef PLAYER_MOVEMENT_H
#define PLAYER_MOVEMENT_H

#include "raylib.h"
#include "raymath.h"
#include "map.hpp"
#include "data_types.hpp"

enum HEADING {
	RIGHT,
	LEFT,
	UP,
	DOWN
};


void CheckMovement1person(Camera *camera, chunkMap map, Mesh *mesh);		// Check for keyboard keys that move player
void CameraMove(Camera *camera, Vector3 direction);		// Translate camera
void CameraRotate(Camera *camera, int HEADING);			// Rotate camera
float getPlayerAngle(Camera camera);			// gets angle of player's voxel to the x axis, angle is in degrees

#endif

