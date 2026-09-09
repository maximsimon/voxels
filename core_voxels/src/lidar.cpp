// ray-casting LiDAR sensor using Amanatides & Woo DDA grid traversal
// rays are cast through the maze_map (mainMap) grid where each cell is 1x1 world unit 
// IMPORTANT: currently each cell is counted as full or empty, smaller "voxels" are sensed by LiDAR as any 1x1x1	TODO figure out
#include "lidar.hpp"
#include "map.hpp"
#include "config_core.hpp"
#include "small_handy_stuff.hpp"
#include "sim_params.hpp"
#include <cmath>

// look up the cell value at absolute grid coordinate (grid_x, grid_z) inside the maze_map, out-of-bounds is treated as empty.
static int cellOccupied(const mainMap &main_map, int grid_x, int grid_z) {
	if (grid_x < 0 || grid_z < 0 || grid_x >= main_map.width_px || grid_z >= main_map.height_px) return 0;                     // treat as empty

	int chunk_x = grid_x / main_map.chunk_side;
	int chunk_z = grid_z / main_map.chunk_side;
	int local_x = grid_x % main_map.chunk_side;
	int local_z = grid_z % main_map.chunk_side;
	int chunk_iray_dir_x = chunk_x + chunk_z * main_map.width_chunks;

	return main_map.chunks[chunk_iray_dir_x].map[local_x + local_z * main_map.chunk_side];
}

// Amanatides & Woo 2D DDA: cast a single ray from origin along angle_rad through the grid of maze_map, returns the hit distance (capped at max_range)
static float castRay(mainMap &main_map, Vector3 origin,	float angle_rad, float max_range) {
	float ray_dir_x = cosf(angle_rad);
	float ray_dir_z = sinf(angle_rad);

	// calculate the starting grid-cell index within maze_map from the world position provided by the caller (origin == player_camera.position)
	int grid_x = (int)origin.x;
	int grid_z = (int)origin.z;
	
	if (cellOccupied(main_map, grid_x, grid_z)) return 0.0f;		// occupied at origin — hit at distance 0

	// DDA step direction per axis
	int step_x = (ray_dir_x > 0.0f) ? 1 : -1;
	int step_z = (ray_dir_z > 0.0f) ? 1 : -1;

	// t-distance to cross one full cell along each axis
	float tDelta_x = (ray_dir_x != 0.0f) ? fabsf(1.0f / ray_dir_x) : INFINITY;
	float tDelta_z = (ray_dir_z != 0.0f) ? fabsf(1.0f / ray_dir_z) : INFINITY;

	// t-distance from origin to the next grid boundary on each axis
	float tMax_x, tMax_z;
	if (ray_dir_x > 0.0f) tMax_x = tDelta_x * (1.0f - (origin.x - (float)grid_x));
	else tMax_x = tDelta_x * (origin.x - (float)grid_x);

	if (ray_dir_z > 0.0f) tMax_z = tDelta_z * (1.0f - (origin.z - (float)grid_z));
	else tMax_z = tDelta_z * (origin.z - (float)grid_z);

	// step through grid cells
	float t = 0.0f;
	while (t < max_range) {
		if (tMax_x < tMax_z) {
			t = tMax_x;
			grid_x += step_x;
			tMax_x += tDelta_x;
		} else {
			t = tMax_z;
			grid_z += step_z;
			tMax_z += tDelta_z;
		}

		if (cellOccupied(main_map, grid_x, grid_z))
			return (t < max_range) ? t : max_range;
		}

	return max_range;
}

// the function called by master_voxel
void updateLidar(VoxelWorld *vw, Observation *observation) {
	Vector3 origin = vw->player_camera.position;
	float heading = getPlayerAngle(vw->player_camera);		// in which direction the robot is facing and so to cast the 1st lidar ray

	// ray count and range are runtime knobs (sim_params); NUM_LIDAR_RAYS stays the
	// compile-time capacity of observation->lidar_scan. Unused slots are zeroed so a
	// consumer reading the full array never sees stale ranges from a previous config.
	const int rays = simActiveLidarRays();
	for (int i = 0; i < rays; i++) {
		float angle = heading + (float)i * (2.0f * PI) / (float)rays;
		observation->lidar_scan[i] = castRay(vw->main_map, origin, angle, sim_params.lidar_range);
	}
	for (int i = rays; i < NUM_LIDAR_RAYS; i++) observation->lidar_scan[i] = 0.0f;
}

