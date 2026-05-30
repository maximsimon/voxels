// Small handy stuff. Convient functions that shorten the main code by few lines

#include "raylib.h"
#include "raymath.h"
#include <stdio.h>
#include "small_handy_stuff.hpp"
#include "data_types.hpp"
#include "config_core.hpp"

Vector3 getUpDirection() {
	Vector3 up = {0.0f, 1.0f, 0.0f};
	return up;
}

Vector3 getDownDirection() {
	Vector3 down = {0.0f, -1.0f, 0.0f};
	return down;
}

Vector3 getRightDirection(Camera camera) {
	Vector3 right = Vector3Normalize(Vector3CrossProduct(getForwardDirection(camera), getUpDirection()));
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

// fetch angle of player in radians
float getPlayerAngle(Camera camera) {
	Vector3 y_axis = {0.0f, 1.0f, 0.0f};
	Vector3 player_vec = Vector3Subtract(camera.target, camera.position);
	
	float angle = atan2f(player_vec.z, player_vec.x);  // angle from +X axis in radians
	return angle;
	
	//float angleDeg = - (angle * 180.0f / PI) - TURN_SPEED * 300;
	//return angleDeg;	
}

// fetch angle of player in degrees
float getPlayerAngleDeg(Camera camera) {
	Vector3 y_axis = {0.0f, 1.0f, 0.0f};
	Vector3 player_vec = Vector3Subtract(camera.target, camera.position);
	
	float angle = atan2f(player_vec.z, player_vec.x);  // angle from +X axis in radians
	float angleDeg = - (angle * 180.0f / PI) - TURN_SPEED * 300;
	return angleDeg;	
}

// print map for debugging purposes
void printMap(mainMap main_map) {
	printf("printing map, width and height in chunks: %d %d\n", main_map.width_chunks, main_map.height_chunks);
	for (int chunk_z = 0; chunk_z < main_map.height_chunks; chunk_z++) {
		for (int chunk_x = 0; chunk_x < main_map.width_chunks; chunk_x++) {
			printf("chunk: %d\n", chunk_x + chunk_z * main_map.width_chunks);
			for (int z = 0; z < chunk_h; z++) {
				for (int x = 0; x < chunk_w; x++) {
					printf("%d ", main_map.chunks[chunk_x + chunk_z * main_map.width_chunks].map[x + z * chunk_w]);
				}
				printf("\n");
			}
		}
		printf("\n");
	}	
}

// visualize lidar on screen
void drawLidarRays(VoxelWorld *vw, Observation *observation, bool in_player_view) {
    float heading = getPlayerAngle(vw->player_camera);
    Vector3 origin = vw->player_camera.position;

    for (int i = 0; i < NUM_LIDAR_RAYS; i++) {
        float range = observation->lidar_scan[i];
        float angle = heading + (float)i * (2.0f * PI) / (float)NUM_LIDAR_RAYS;
        Vector3 end = {
            origin.x + cosf(angle) * range,
            0.5f,
            origin.z + sinf(angle) * range
        };
        DrawLine3D(origin, end, RED);
    }
}

