// ray-casting LiDAR sensor using Amanatides & Woo DDA grid traversal

#ifndef LIDAR_H
#define LIDAR_H

#include "raylib.h"
#include "data_types.hpp"

// look up the cell value at absolute grid coordinate (grid_x, grid_z) inside the maze_map, out-of-bounds is treated as empty.
static int cellOccupied(const mainMap &main_map, int grid_x, int grid_z);
// Amanatides & Woo 2D DDA: cast a single ray from origin along angle_rad through the grid of maze_map, returns the hit distance (capped at max_range)
static float castRay(mainMap &main_map, Vector3 origin,	float angle_rad, float max_range);
// the function called by master_voxel
void updateLidar(VoxelWorld *vw, Observation *observation);

#endif
