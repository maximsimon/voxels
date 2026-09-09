#ifndef WORLD_CONFIG_H
#define WORLD_CONFIG_H

#include "data_types_worlds.hpp"

// parse config file (worlds.config) and set CurrentWorld based on it - which world is used
void load_world_config(const char *config_path);

// look a world up by its name ("ConferenceWorld" | "SidlisteWorld" | "DesertWorld" | "BleakCityWorld").
// writes it to *out and returns true; returns false and leaves *out alone for an unknown name.
bool world_by_name(const char *name, World *out);

// inverse of world_by_name: the name of a World, or "" if it is not one of the predefined ones
const char *world_name_of(const World &world);

// comma-separated list of the names world_by_name() accepts - for error messages
const char *world_names(void);

#endif
