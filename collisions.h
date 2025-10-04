// Tools for collision checking, e.g. calculating current boundry points of a voxel.

#ifndef COLLISIONS_H
#define COLLISIONS_H

#include "raylib.h"
#include "raymath.h"
#include <stdio.h>
#include "map.h"
#include "small_handy_stuff.h"

bool CheckCollision(Camera *camera, chunkMap map, const float speed, Vector3 directioni, Mesh *mesh);		// Check if player is colliding with any voxel
void getVoxelBoundryPoints(Vector3 *boundry_points, Vector3 pose, chunkMap map);		// calculate position 4 vertices of voxel (top square) 

// calculate position 4 vertices of voxel (top square) 
void getVoxelBoundryPoints(Vector3 *boundry_points, Vector3 pose, chunkMap map) {


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
bool CheckCollision(Camera *camera, chunkMap map, const float speed, Vector3 direction, Mesh *mesh) {

	bool collision = false;
	float buffer = 1.0f;

	// find in which voxel player is going to be located
	

	Vector3 forward = getForwardDirection(*camera);
	Vector3 right = getRightDirection(*camera);
	
	Vector3 future_pose = Vector3Add(camera->position, Vector3Scale(direction, speed));
	future_pose = Vector3Scale(future_pose, buffer);
	
	Vector3 boundry_points[40];
	getVoxelBoundryPoints(boundry_points, future_pose, map);
	for (int i = 0; i < 40; i++) {
		boundry_points[i] = Vector3Add(future_pose, boundry_points[i]);
	}
	
	
	// Check collision by looking at the map. doesn't really work well.
	//int map_x = 0;
	//int map_z = 0; 
	////check if that voxel is air or matter
	//for (int pt_idx = 0; pt_idx < 40; pt_idx++) {
	//	map_x = round(boundry_points[pt_idx].x);
	//	map_z = round(boundry_points[pt_idx].z);
	//	if ((0 < map_x) && (16 > map_x) && (0 < map_z) && (16 > map_z) && (map.map[map_x + map_z * 16] == 1)) {
	//		collision = true;
	//		printf("happened, map_x, map_z: %d %d", map_x, map_z);
	//		break;
	//	}
	//}

	// Check collision by shooting ray up and checking if it collides with mesh
	Ray ray = { 0 };
	ray.position = (Vector3){0.0f, 0.0f, 0.0f};
	ray.direction = (Vector3){ 0.0f, 1.0f, 0.0f };

	RayCollision ray_collision = { 0 };

	for (int pt_idx = 0; pt_idx < 40; pt_idx++) {
		ray.position = boundry_points[pt_idx];
		printf("ray position: %f %f %f \n ", ray.position.x, ray.position.y, ray.position.z);
		ray_collision = GetRayCollisionMesh(ray, *mesh, MatrixIdentity());	
		if (ray_collision.hit == true) break;
	}
	
	collision = ray_collision.hit;
	printf("collision: %d \n", collision);
	
	return collision;

}



#endif
