#ifndef CONFIG_H
#define CONFIG_H

#include <string>

// input device to controll the player (e.g. keyboard) - set MANUALLY_SET_DEVICE to 1 to use DEVICE value, 0 to find device automatically in the code (teleop_keys.cpp)
inline bool MANUALLY_SET_DEVICE = 0;
inline std::string DEVICE = "/dev/input/event4";

// forward velocity (m/s) and yaw rate (rad/s) when the relevant key is held
float ROS_SPEED = 3.0;          // m/s forward
float ROS_TURN_SPEED = 1.0;     // rad/s 

#endif 
