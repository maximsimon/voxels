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


bool CheckMovement1person(Camera *camera, chunkMap map, Mesh *mesh, Observation *observation);		// Check for keyboard keys that move player
void CameraMove(Camera *camera, Vector3 direction, float speed);		// Translate camera
void CameraRotate(Camera *camera, int HEADING, float turn_speed, Observation *observation);			// Rotate camera
float getPlayerAngle(Camera camera);			// gets angle of player's voxel to the x axis, angle is in degrees
void teleport(Camera *camera, Vector3 goal_pose);		// teleport whatever camera (player or god) to goal_pose, without changing orientation
void teleportGUIinput(VoxelWorld *vw);			// teleport camera based on in simulation input (press T and enter goal pose into textboxt)

#endif

