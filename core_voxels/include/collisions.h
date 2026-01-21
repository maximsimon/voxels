// Tools for collision checking, e.g. calculating current boundry points of a voxel.

#ifndef COLLISIONS_H
#define COLLISIONS_H

#include "raylib.h"
#include "raymath.h"
#include "map.h"

#ifdef __cplusplus
extern "C" {
#endif

bool CheckCollision(Camera *camera, chunkMap map, const float speed, Vector3 directioni, Mesh *mesh);		// Check if player is colliding with any voxel
void getVoxelBoundryPoints(Vector3 *boundry_points, Vector3 pose, chunkMap map);		// calculate position 4 vertices of voxel (top square) 

#ifdef __cplusplus
}
#endif

#endif
