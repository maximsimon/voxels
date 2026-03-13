// UTILS FOR MASTER_VOXEL.C 

#ifndef SUPPORT_FOR_MASTER_H
#define SUPPORT_FOR_MASTER_H

#include "raylib.h"
#include "raymath.h"
#include "master_voxel.hpp"
#include "data_types.hpp"

void movePlayerWithAction(VoxelWorld *vw, int curr_chunk, Action *action);
void handleActionsAndKeys(VoxelWorld *vw, int curr_chunk, Action *action);
bool checkControls(VoxelWorld *vw, int curr_chunk);

#endif

