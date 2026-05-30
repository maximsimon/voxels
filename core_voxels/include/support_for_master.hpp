// UTILS FOR MASTER_VOXEL.C 

#ifndef SUPPORT_FOR_MASTER_H
#define SUPPORT_FOR_MASTER_H

#include "raylib.h"
#include "raymath.h"
#include "master_voxel.hpp"
#include "data_types.hpp"

void movePlayerWithAction(VoxelWorld *vw, Action *action, Observation *observation);
void handleActionsAndKeys(VoxelWorld *vw, Action *action, Observation *observation);
bool checkControls(VoxelWorld *vw, Action *action, Observation *observation);

#endif

