#ifndef CONFIG_CORE_H
#define CONFIG_CORE_H

#include "worlds.hpp"

// simulation window size
inline const int SCREEN_WIDTH = 1600;
inline const int SCREEN_HEIGHT = 850;

// sped of movement and rotation
inline const float SPEED = 0.05f;	
inline const float TURN_SPEED = 0.01f;

inline const float SPEED_GOD = 0.5f;
inline const float TURN_SPEED_GOD = 0.05f;

// lidar constants
inline const int NUM_LIDAR_RAYS = 180;
inline const float MAX_LIDAR_RANGE = 40.0f;

// STUFF FOR MAP.HPP AND BUILDING THE WORLD IN GENERAL


// world dimension
inline const int CHUNK_WIDTH = 16;
inline const int CHUNK_HEIGHT = 16;


#endif 

