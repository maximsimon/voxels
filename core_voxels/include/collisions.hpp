// Tools for collision checking, e.g. calculating current boundry points of a voxel.

#ifndef COLLISIONS_H
#define COLLISIONS_H

#include "raylib.h"
#include "raymath.h"
#include "data_types.hpp"

bool CheckCollision(VoxelWorld *vw, Camera *camera, const float speed, Vector3 direction, Mesh *mesh);		// Check if player is colliding with any voxel
void getVoxelBoundryPoints(Vector3 *boundry_points, Vector3 pose);		// calculate position 4 vertices of voxel (top square) 

#endif
