// THIS IS MAIN
// VOXELS: PROGRAMMING MY RENDERING OF VOXEL WORLD, starting with a MAZE FROM PICTURE AND MY KEYBINDINGS FOR MOVEMENT, then connecting to ROS and more

#ifndef MASTER_VOXEL_H
#define MASTER_VOXEL_H

#include "raylib.h"
#include "faces.hpp"
#include "data_types.hpp"

VoxelWorld *init_sim(Vector3 player_pose, Vector3 player_direction, int step_fq);			 // initilize simulation - allocate memory, create structs, define window size, etc
void step_sim(VoxelWorld *vw, Action *action, Observation *observation);			// this function is one step of simulation, here happen all calculations of what happens in the simulation as well as the actual visual rendering, called in loop from master_main
static void drawPlayer(Camera3D camera, Color color); 		// Draw an arrow at the agent's position pointing along its look direction

#endif
