// THIS IS MAIN
// VOXELS: PROGRAMMING MY RENDERING OF VOXEL WORLD, starting with a MAZE FROM PICTURE AND MY KEYBINDINGS FOR MOVEMENT, then connecting to ROS and more

#ifndef MASTER_VOXEL_H
#define MASTER_VOXEL_H

#include "raylib.h"
#include "faces.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct VoxelWorld VoxelWorld;	// struct that holds all info about simulation that is necassary to rembebr between steps
typedef struct Observation Observation;
typedef struct Action Action;

// window size
const int screen_width = 1600;
const int screen_height = 850;

VoxelWorld *init_sim(Image mazemap_image, Vector3 player_pose, Vector3 player_direction, int step_fq);
Observation *step_sim(VoxelWorld *vw, Action *action);

#ifdef __cplusplus
}
#endif

#endif
