// VOXELS: THIS IS MAIN FILE CONTROLLING THE VOXELS SIMULATION

#ifndef MASTER_VOXEL_H
#define MASTER_VOXEL_H

#include "raylib.h"
#include "faces.hpp"
#include "data_types.hpp"

// initilize simulation - allocate memory, create structs, define window size, etc.
VoxelWorld *init_sim(Vector3 player_pose, Vector3 player_direction);

// one step of simulation: all calculations of what happens in the simulation plus the visual rendering; called in a loop from master_main.
void step_sim(VoxelWorld *vw, Action *action, Observation *observation);

// tear the simulation down - frees the world, the player model and the render texture, then closes the window
void end_sim(VoxelWorld *vw);

#endif
