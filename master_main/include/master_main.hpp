// DEFINITIONS AND SUCH FOR MASTER_MAIN

#ifndef MASTER_MAIN_H
#define MASTER_MAIN_H

#include "data_types.hpp"

// external action/observation buffers - the hand-off point between the simulation and external modules (e.g. the main)
extern Observation master_observation;
extern Action master_action;

// initializes the entire Voxel World simulation - called once at launch.
// sim_params (sim_params.hpp) must be set before this: the window size, camera resolution and window visibility are read here and cannot change afterwards.
void master_init_sim();

// steps one simulation frame - called repeatedly in a loop; returns false when the window/quit key requests exit
bool master_step_sim();

// puts the robot back at (x, z) facing yaw_rad, optionally switching world first; world_name may be NULL or "" to stay in the current world
bool master_reset_sim(const char *world_name, float x, float z, float yaw_rad);

// name of the world currently built, or "" before init
const char *master_current_world_name();

// tears the simulation down - frees the world and closes the window
void master_end_sim();

// teleport channel - xternal modules queue a request; coordinates are in raylib floor-plane axes
void master_main_teleport(float x, float z, float yaw_rad);
bool master_consume_teleport(float *x, float *z, float *yaw_rad);

#endif
