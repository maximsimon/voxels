#ifndef CONFIG_MASTER_H
#define CONFIG_MASTER_H

#include "raylib.h"

// initial player position and heading direction, used by master_init_sim().
// PLAYER_DIRECTION_INIT is a direction vector added to the position to get the camera
// target, so only its x/z components decide the starting heading.
inline Vector3 PLAYER_POSE_INIT = { -2.0f, 0.5f, -2.0f };
inline Vector3 PLAYER_DIRECTION_INIT = { -2.0f, 0.5f, -1.0f };

#endif
