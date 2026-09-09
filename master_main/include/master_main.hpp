// DEFINITIONS AND SUCH FOR MASTER_MAIN

#ifndef MASTER_MAIN_H
#define MASTER_MAIN_H

#include "data_types.hpp"

// public action/observation buffers - the hand-off point between the simulation
// and external modules (e.g. the python bridge). Write master_action before a
// step, read master_observation after it.
extern Observation master_observation;
extern Action master_action;

// initializes the entire Voxel World simulation - called once at launch.
// sim_params (sim_params.hpp) must be set before this: the window size, camera
// resolution and window visibility are read here and cannot change afterwards.
void master_init_sim();

// steps one simulation frame - called repeatedly in a loop from src/main.cpp; returns false when the window/quit key requests exit
bool master_step_sim();

// Re-place the robot at (x, z) with heading yaw_rad, optionally switching the world
// first.  world_name is one of the names world_by_name() accepts, or NULL/"" to stay
// in the current world; the world is only rebuilt when it actually changes, so a reset
// inside one world costs a teleport.  Pending teleports, the action buffer and the
// reported velocities are cleared so the next step starts from a clean slate.
// Returns false without changing anything if world_name is not a known world.
bool master_reset_sim(const char *world_name, float x, float z, float yaw_rad);

// name of the world currently built, or "" before init
const char *master_current_world_name();

// tears the simulation down - frees the world and closes the window
void master_end_sim();

// Teleport channel.  External modules queue a request via
// master_ros_teleport(); master_step_sim() drains it once per frame via
// master_consume_teleport().  Coordinates are in raylib floor-plane axes
// (x along raylib +x, z along raylib +z); yaw_rad is the heading angle in
// the X-Z plane measured from +x (matches getPlayerAngle).
void master_ros_teleport(float x, float z, float yaw_rad);
bool master_consume_teleport(float *x, float *z, float *yaw_rad);

#endif
