#ifndef WORLD_LOADING_H
#define WORLD_LOADING_H

#include "data_types.hpp"
#include "data_types_worlds.hpp"

// build the map, meshes, models and textures of vw->current_world.
// vw->current_world must already be set; anything previously built must already be
// destroyed (build_world does not free the old world - use switch_world / reload_world).
void build_world(VoxelWorld *vw);

// release everything build_world() allocated (CPU map + GPU meshes/models/textures).
// safe to call on a vw that has nothing built.
void destroy_world(VoxelWorld *vw);

// re-read worlds.config and rebuild - for live changing of world appearance (the R key)
void reload_world(VoxelWorld *vw);

// switch to a named world (see world_by_name).  Rebuilds only when the world actually
// differs from the current one, so resetting inside the same world stays cheap.
// returns false (and leaves the world untouched) for an unknown name.
bool switch_world(VoxelWorld *vw, const char *world_name);

void load_world_config(const char *config_path);		// parse config file (worlds.config) and set CurrentWorld based on which world is used.

#endif
