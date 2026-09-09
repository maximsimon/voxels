// THIS IS MAIN
// VOXELS: PROGRAMMING MY RENDERING OF VOXEL WORLD, starting with a MAZE FROM PICTURE AND MY KEYBINDINGS FOR MOVEMENT, then connecting to ROS and more

#ifndef MASTER_VOXEL_H
#define MASTER_VOXEL_H

#include "raylib.h"
#include "faces.hpp"
#include "data_types.hpp"

// initilize simulation - allocate memory, create structs, define window size, etc.
// Window size, frame-rate cap and camera resolution come from sim_params (sim_params.hpp)
// and have to be set before calling this.
VoxelWorld *init_sim(Vector3 player_pose, Vector3 player_direction);

// one step of simulation: all calculations of what happens in the simulation plus the
// visual rendering. Called in a loop from master_main.
void step_sim(VoxelWorld *vw, Action *action, Observation *observation);

// tear the simulation down - frees the world, the player model and the render texture, then closes the window
void end_sim(VoxelWorld *vw);

#endif
