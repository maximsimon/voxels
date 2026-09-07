#ifndef WORLD_LOADING_H
#define WORLD_LOADING_H

#include "data_types.hpp"

void reload_world(VoxelWorld *vw);		// reload world - for live changing of world appearance
void load_world_config(const char *config_path);		// parse config file (worlds.config) and set CurrentWorld based on which world is used.

#endif
