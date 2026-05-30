// Tools for collision checking, e.g. calculating current boundry points of a voxel.

#include "raylib.h"
#include "raymath.h"
#include <stdio.h>
#include "small_handy_stuff.hpp"
#include "collisions.hpp"
#include "data_types.hpp"
#include "map.hpp"

// calculate position 4 vertices of voxel (top square) 
void getVoxelBoundryPoints(Vector3 *boundry_points, Vector3 pose) {

	double value = -0.5;
	for (int i = 0; i < 10; i++) { 
		boundry_points[i*4].x = 0.5;
		boundry_points[i*4].y = -0.5f;
		boundry_points[i*4].z = value;

		boundry_points[i*4+1].x = 0.5;
		boundry_points[i*4+1].y = -0.5f;
		boundry_points[i*4+1].z = -value;
		
		boundry_points[i*4+2].x = -0.5;
		boundry_points[i*4+2].y = -0.5f;
		boundry_points[i*4+2].z = value;
		
		boundry_points[i*4+3].x = -0.5;
		boundry_points[i*4+3].y = -0.5f;
		boundry_points[i*4+3].z = -value;
		value += 0.1;
	}

}

// Check if player is colliding with any voxel
bool CheckCollision(VoxelWorld *vw, Camera *camera, const float speed, Vector3 direction, Mesh *mesh) {

	bool collision = false;
	float buffer = 1.0f;

	// find in which voxel player is going to be located
	
	Vector3 forward = getForwardDirection(*camera);
	Vector3 right = getRightDirection(*camera);
	
	Vector3 future_pose = Vector3Add(camera->position, Vector3Scale(direction, speed));
	future_pose = Vector3Scale(future_pose, buffer);
	
	Vector3 boundry_points[40];
	getVoxelBoundryPoints(boundry_points, future_pose);
	for (int i = 0; i < 40; i++) {
		boundry_points[i] = Vector3Add(future_pose, boundry_points[i]);
	}
	
	// Check collision by shooting ray up and checking if it collides with mesh
	Ray ray = { 0 };
	ray.position = (Vector3){0.0f, 0.0f, 0.0f};
	ray.direction = (Vector3){ 0.0f, 1.0f, 0.0f };

	RayCollision ray_collision = { 0 };

	int chunk_coord = 0;
	for (int pt_idx = 0; pt_idx < 40; pt_idx++) {
		ray.position = boundry_points[pt_idx];
		chunk_coord = fetchCurrChunkIdx(vw->main_map, boundry_points[pt_idx]);
		ray_collision = GetRayCollisionMesh(ray, mesh[chunk_coord], MatrixIdentity());	
		if (ray_collision.hit == true) break;
	}
	
	collision = ray_collision.hit;
	
	return collision;

}


