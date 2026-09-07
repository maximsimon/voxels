// Small handy stuff. Convient functions that shorten the main code by few lines

#ifndef SMALL_HANDY_STUFF_H
#define SMALL_HANDY_STUFF_H

#include "raylib.h"
#include "raymath.h"
#include "data_types.hpp"

Vector3 getUpDirection();
Vector3 getDownDirection();
Vector3 getRightDirection(Camera camera);
Vector3 getLeftDirection(Camera camera);
Vector3 getBackDirection(Camera camera);
Vector3 getForwardDirection(Camera camera);
float getPlayerAngle(Camera camera);		// heading angle of the camera in XZ plane, measured from +X axis, radians
float getPlayerAngleDeg(Camera camera);		// heading angle ---------------------- || --------------------- , degrees
void printMap(mainMap main_map);	// helper debugging function for printing map
void drawLidarRays(VoxelWorld *vw, Observation *observation, bool in_player_view);		// visualize lidar on screen
void printWorld(const World& world);			// convinience function for printing World structure (e.g. definition of currently rendered world: printWorld(vw->current_world))

#endif
