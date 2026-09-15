// PARSES WORLD CONFIG FILE

#include "world_config.hpp"

#include <cctype>
#include <cstdio>
#include <cstring>
#include <string>
#include <utility>
#include <vector>

#include "data_types_worlds.hpp"
#include "world_registry.hpp"
#include "sim_params.hpp"

// parse config file (worlds.config) and set CurrentWorld based on which world is used.
// config format: lines of "key=value", '#' starts a comment, blank lines skipped.
// keys: "registry" -> the registry file that replaces the whole world table (REPLACE semantics),
//       "world" -> any world in that table (classic or registry-loaded) becomes CurrentWorld.
// absent a "registry" key the classic worlds.registry is loaded, so a bare config still works.

#define DEFAULT_WORLD_REGISTRY "core_voxels/resources/worlds/worlds.registry"

// look a world up by name - delegated to the registry, the single place the name -> World mapping lives.
bool world_by_name(const char *name, World *out) {
	return registry_world_by_name(name, out);
}

// worlds are compared by their registered NAME - each registered world has one.
const char *world_name_of(const World &world) {
	return registry_world_name_of(world);
}

std::string world_names(void) {
	return registry_world_names();
}

void load_world_config(const char *config_path) {
	FILE *fp = fopen(config_path, "r");
	if (fp == NULL) {
		printf("world_config: could not open config file '%s', keeping current world\n", config_path);
		return;
	}

	// buffer every non-blank, non-comment "key=value" line so "registry" is applied before "world" regardless of file order.
	std::vector<std::pair<std::string, std::string>> entries;
	char line[256];
	while (fgets(line, sizeof(line), fp) != NULL) {
		size_t len = strlen(line);
		while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) line[--len] = '\0';
		if (len == 0 || line[0] == '#') continue;

		char *eq = strchr(line, '=');
		if (eq == NULL) continue;
		*eq = '\0';
		char *key = line;
		char *value = eq + 1;

		// trim whitespace around key and value
		while (isspace((unsigned char)*key)) key++;
		char *k_end = key + strlen(key) - 1;
		while (k_end >= key && isspace((unsigned char)*k_end)) k_end--;
		k_end[1] = '\0';
		while (isspace((unsigned char)*value)) value++;
		char *v_end = value + strlen(value) - 1;
		while (v_end >= value && isspace((unsigned char)*v_end)) v_end--;
		v_end[1] = '\0';

		entries.emplace_back(key, value);
	}
	fclose(fp);

	// apply "registry" first - it replaces the whole world table; absent it the classics are the table.
	// skip the default load when the table is already non-empty (e.g. ReloadRegistry before Init).
	bool registry_set = false;
	for (const auto &entry : entries) {
		if (entry.first == "registry") {
			load_world_registry(entry.second.c_str());
			registry_set = true;
		}
	}
	if (!registry_set && registry_world_count() == 0) load_world_registry(DEFAULT_WORLD_REGISTRY);

	// then match "world" to a registered world (classic or registry-loaded).
	for (const auto &entry : entries) {
		if (entry.first != "world") continue;
		if (!world_by_name(entry.second.c_str(), &CurrentWorld)) {
			printf("world_config: unknown world '%s', keeping current world\n", entry.second.c_str());
			continue;
		}
		if (sim_params.verbose) printf("world_config: world set to %s\n", entry.second.c_str());
		return;
	}

	// fall back to the first registered world when the active one is gone (registry was replaced).
	const char *old_name = CurrentWorld.NAME;
	World current_probe;
	if (CurrentWorld.NAME == nullptr || !world_by_name(CurrentWorld.NAME, &current_probe)) {
		World first;
		if (registry_world_count() > 0 && registry_world_by_name(registry_world_name_at(0), &first)) {
			CurrentWorld = first;
			if (sim_params.verbose) printf("world_config: '%s' not in the registry, defaulting to the first registered world '%s'\n",
			                               old_name != nullptr ? old_name : "", first.NAME);
			return;
		}
	}

	printf("world_config: no world selected from '%s' and no registered world to fall back on\n", config_path);
}