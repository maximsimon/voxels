#include "world_config.hpp"

#include <cctype>
#include <cstdio>
#include <cstring>
#include <unordered_map>
#include <vector>

#include "worlds.hpp"

// parse config file (worlds.config) and set CurrentWorld based on which world is used.
// config format: lines of "key=value", '#' starts a comment, blank lines skipped.
// the only param is "world" -> one of ConferenceWorld | SidlisteWorld | DesertWorld | BleakCityWorld

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
		if      (strcmp(value, "ConferenceWorld") == 0) CurrentWorld = ConferenceWorld;
		else if (strcmp(value, "SidlisteWorld")   == 0) CurrentWorld = SidlisteWorld;
		else if (strcmp(value, "DesertWorld")     == 0) CurrentWorld = DesertWorld;
		else if (strcmp(value, "BleakCityWorld")  == 0) CurrentWorld = BleakCityWorld;
		else {
			printf("world_config: unknown world '%s', keeping default world (DesertWorld)\n", value);
			continue;
		}

		printf("world_config: world set to %s\n", value);
		fclose(fp);
		return;
	}

	fclose(fp);
	printf("world_config: no valid 'world=' param found in '%s', keeping default world (DesertWorld)\n", config_path);
}
