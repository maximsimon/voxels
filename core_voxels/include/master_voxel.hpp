// THIS IS MAIN
// VOXELS: PROGRAMMING MY RENDERING OF VOXEL WORLD, starting with a MAZE FROM PICTURE AND MY KEYBINDINGS FOR MOVEMENT, then connecting to ROS and more

#ifndef MASTER_VOXEL_H
#define MASTER_VOXEL_H

#include "raylib.h"
#include "faces.hpp"
#include "data_types.hpp"

VoxelWorld *init_sim(Image mazemap_image, Vector3 player_pose, Vector3 player_direction, int step_fq);
Observation *step_sim(VoxelWorld *vw, Action *action);

#endif
