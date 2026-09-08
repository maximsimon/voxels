// DEFINITIONS AND SUCH FOR MASTER_MAIN 

#ifndef MASTER_MAIN_H
#define MASTER_MAIN_H

#include "data_types.hpp"

// public action/observation buffers - the hand-off point between the simulation
// and external modules (e.g. the python bridge). Write master_action before a
// step, read master_observation after it.
extern Observation master_observation;
extern Action master_action;

// initializes the entire Voxel World simulation - called once at launch
void master_init_sim();

// steps one simulation frame - called repeatedly in a loop from src/main.cpp; returns false when the window/quit key requests exit
bool master_step_sim();

// Teleport channel.  External modules queue a request via
// master_ros_teleport(); master_step_sim() drains it once per frame via
// master_consume_teleport().  Coordinates are in raylib floor-plane axes
// (x along raylib +x, z along raylib +z); yaw_rad is the heading angle in
// the X-Z plane measured from +x (matches getPlayerAngle).
void master_ros_teleport(float x, float z, float yaw_rad);
bool master_consume_teleport(float *x, float *z, float *yaw_rad);

#endif
