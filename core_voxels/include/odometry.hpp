// HANDLE ODOMETRY, WHICH IS PART OF OBSERVATION

#ifndef ODOMETRY_H
#define ODOMETRY_H

#include "raylib.h"
#include "data_types.hpp"

void updateOdometry(VoxelWorld *vw, Action *action, Observation *observation);
Quaternion getQuaternionOrientation(VoxelWorld *vw);

#endif
