#include "world_config.hpp"

#include <cctype>
#include <cstdio>
#include <cstring>
#include <unordered_map>
#include <vector>

#include "worlds.hpp"
#include "sim_params.hpp"

// parse config file (worlds.config) and set CurrentWorld based on which world is used.
// config format: lines of "key=value", '#' starts a comment, blank lines skipped.
// the only param is "world" -> one of ConferenceWorld | SidlisteWorld | DesertWorld | BleakCityWorld

// look a world up by name - the single place the name -> World mapping lives, so the
// config file parser and the programmatic reset() path cannot drift apart.
bool world_by_name(const char *name, World *out) {
	if (name == NULL || out == NULL) return false;

	if      (strcmp(name, "ConferenceWorld") == 0) *out = ConferenceWorld;
	else if (strcmp(name, "SidlisteWorld")   == 0) *out = SidlisteWorld;
	else if (strcmp(name, "DesertWorld")     == 0) *out = DesertWorld;
	else if (strcmp(name, "BleakCityWorld")  == 0) *out = BleakCityWorld;
	else return false;

	return true;
}

// worlds are compared by map image path - it is what uniquely identifies each predefined world
const char *world_name_of(const World &world) {
	if (world.MAP_IMAGE_PATH == NULL) return "";
	if (strcmp(world.MAP_IMAGE_PATH, ConferenceWorld.MAP_IMAGE_PATH) == 0) return "ConferenceWorld";
	if (strcmp(world.MAP_IMAGE_PATH, SidlisteWorld.MAP_IMAGE_PATH)   == 0) return "SidlisteWorld";
	if (strcmp(world.MAP_IMAGE_PATH, DesertWorld.MAP_IMAGE_PATH)     == 0) return "DesertWorld";
	if (strcmp(world.MAP_IMAGE_PATH, BleakCityWorld.MAP_IMAGE_PATH)  == 0) return "BleakCityWorld";
	return "";
}

const char *world_names(void) {
	return "ConferenceWorld, SidlisteWorld, DesertWorld, BleakCityWorld";
}

void load_world_config(const char *config_path) {
	FILE *fp = fopen(config_path, "r");
	if (fp == NULL) {
		printf("world_config: could not open config file '%s', keeping default world (DesertWorld)\n", config_path);
		return;
	}

	char line[256];
	while (fgets(line, sizeof(line), fp) != NULL) {
		// strip trailing newline/carriage return
		size_t len = strlen(line);
		while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) line[--len] = '\0';

		// skip blank lines and comments
		if (len == 0 || line[0] == '#') continue;

		// split "key=value" on '='
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

		if (strcmp(key, "world") != 0) continue;

		// match world name to the predefined world in worlds.hpp
		if (!world_by_name(value, &CurrentWorld)) {
			printf("world_config: unknown world '%s', keeping default world (DesertWorld)\n", value);
			continue;
		}

		if (sim_params.verbose) printf("world_config: world set to %s\n", value);
		fclose(fp);
		return;
	}

	fclose(fp);
	printf("world_config: no valid 'world=' param found in '%s', keeping default world (DesertWorld)\n", config_path);
}
