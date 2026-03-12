// DEFINITIONS AND SUCH FOR MASTER_MAIN 

#ifndef MASTER_MAIN_H
#define MASTER_MAIN_H

#include <thread>
#include <mutex>
#include "data_types.hpp"

// entire Voxel World is initiated and ran from this function, which is called by master_main.cpp main() function in its own neat little thread (hopefully)
void master_step_sim();

//funkce co ma action v argumentu a vraci observation (ros vola tuhle funkci)
//Observation *master_ros_bridge(Action *ros_action);
void master_ros_bridge(Action *ros_action, Observation *ros_observation);

#endif
