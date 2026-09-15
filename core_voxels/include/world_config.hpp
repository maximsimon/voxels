#ifndef WORLD_CONFIG_H
#define WORLD_CONFIG_H

#include "data_types_worlds.hpp"
#include <string>

// parse config file (worlds.config) and set CurrentWorld based on it.
// "registry" replaces the whole world table (REPLACE semantics); "world" picks the active one.
void load_world_config(const char *config_path);

// look a world up by its name; writes it to *out and returns true, leaves *out alone for an unknown name.
bool world_by_name(const char *name, World *out);

// inverse of world_by_name: the name of a World, or "" if it is not a registered one
const char *world_name_of(const World &world);

// comma-separated list of the names world_by_name() accepts - for error messages
std::string world_names(void);

#endif
