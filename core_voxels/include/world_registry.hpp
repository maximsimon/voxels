#ifndef WORLD_REGISTRY_H
#define WORLD_REGISTRY_H

#include "data_types_worlds.hpp"
#include <string>

// the runtime name -> World table the simulator switches by.
// load_world_registry() replaces the whole table with the worlds in a plain-text registry file.
// worlds from an earlier load are gone until that file (or another) is loaded again.

void load_world_registry(const char *registry_path);

// look a world up by name in the registry; writes it to *out for a hit, returns false otherwise.
bool registry_world_by_name(const char *name, World *out);

// the name of a World, or "" when it has none.
const char *registry_world_name_of(const World &world);

// number of worlds currently in the registry.
int registry_world_count(void);

// name of the i-th registered world (registry order), "" when out of range.
const char *registry_world_name_at(int i);

// comma-separated list of every registered name - for error messages.
std::string registry_world_names(void);

#endif