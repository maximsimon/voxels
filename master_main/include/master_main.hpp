// DEFINITIONS AND SUCH FOR MASTER_MAIN 

#ifndef MASTER_MAIN_H
#define MASTER_MAIN_H

#include <thread>
#include <mutex>
#include "data_types.hpp"

// entire Voxel World is initiated and ran from this function, which is called by master_main.cpp main() function in its own neat little thread (hopefully)
void master_step_sim();

// function called from ros_voxels, ros passes Action to control the robot and recieves Observation to see sensor readings from simulation
void master_ros_bridge(Action *ros_action, Observation *ros_observation);

// Cross-thread teleport channel.  ROS callbacks queue a request via master_ros_teleport(); the simulation thread drains it once per frame via master_consume_teleport().
// Coordinates are in raylib floor-plane axes x, z; yaw_rad is the heading angle in the X-Z plane measured from +x (matches getPlayerAngle).
void master_ros_teleport(float x, float z, float yaw_rad);
bool master_consume_teleport(float *x, float *z, float *yaw_rad);

#endif
