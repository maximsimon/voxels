// RUNTIME WORLD REGISTRY - the name -> World table the simulator switches by.
// load_world_registry() parses a text registry file; REPLACE semantics - the file is the whole table until the next load.
// every C string a registered World exports is owned here, for the process lifetime.

#include "world_registry.hpp"

#include <cstring>
#include <deque>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "objects.hpp"
#include "color_hue_mapping.hpp"
#include "sim_params.hpp"

// string storage owned by the registry: World's const char* members point in here.
// a std::deque never moves existing entries, so those pointers stay valid across appends.
static std::deque<std::string> registry_strings;
static std::vector<World> registry_worlds;

// keep a string alive for the process lifetime and hand back a stable pointer into it.
static const char *own_string(const char *s) {
	registry_strings.emplace_back(s != nullptr ? s : "");
	return registry_strings.back().c_str();
}

// resolve a registry color name to its hue type + hue center; unknown when not a color name.
static HUE_TYPE color_from_name(const char *name, float *hue_deg) {
	HUE_TYPE t = hue_by_name(name);
	if (t == unknown) return unknown;
	*hue_deg = color_center_of(t);
	return t;
}

// the raw key=value settings parsed for one world block, before they become a World struct.
struct PendingWorld {
	std::string name, map, atlas, ground, sky;
	int atlas_cols = 2, atlas_rows = 2;
	std::vector<std::string> colors;				// palette the map image was drawn with
	std::unordered_map<HUE_TYPE, int> tile;			// hue -> atlas tile (fallback textures)
	std::unordered_map<std::string, int> object_texture;	// object name -> explicit atlas tile
	std::unordered_map<HUE_TYPE, std::string> objects;	// hue -> object library name
};

// folded raw settings into a World; returns false when a required key is missing (world is skipped)
static bool make_world(const PendingWorld &p, World *out) {
	std::string missing;
	if (p.map.empty())    missing += " map";
	if (p.atlas.empty())  missing += " atlas";
	if (p.ground.empty()) missing += " ground";
	if (p.sky.empty())    missing += " sky";
	if (!missing.empty()) {
		printf("world_registry: skipping '%s' - missing required key(s):%s\n", p.name.c_str(), missing.c_str());
		return false;
	}

	World w = {};
	w.NAME = own_string(p.name.c_str());
	w.MAP_IMAGE_PATH = own_string(p.map.c_str());
	w.TEXTURE_ATLAS_PATH = own_string(p.atlas.c_str());
	w.GROUND_TEXTURE_PATH = own_string(p.ground.c_str());
	w.SKY_TEXTURE_PATH = own_string(p.sky.c_str());
	w.atlas_cols = p.atlas_cols;
	w.atlas_rows = p.atlas_rows;

	HueConfig cfg;		// named colors from the file get their default hue center
	cfg.hue_tolerance_deg = 45.0f;
	for (const std::string &color : p.colors) {
		float hue_deg = 0.0f;
		HUE_TYPE t = color_from_name(color.c_str(), &hue_deg);
		if (t != unknown) cfg.colors.push_back({t, hue_deg});
	}
	w.hue_config = cfg;

	w.hue_tile = p.tile;
	for (const auto &entry : p.objects) {
		const ObjectDefinition *def = object_by_name(entry.second.c_str());
		if (def == nullptr) continue;		// unknown object name - skip that mapping
		ObjectDefinition obj = *def;
		auto tex_it = p.object_texture.find(entry.second);
		if (tex_it != p.object_texture.end()) obj.texture = tex_it->second;
		w.objects[entry.first] = obj;
	}

	*out = w;
	return true;
}

// add one parsed world to the registry; duplicate names keep the first definition.
static void register_pending(const PendingWorld &p) {
	if (p.name.empty()) return;
	for (const World &existing : registry_worlds) {
		if (existing.NAME != nullptr && strcmp(existing.NAME, p.name.c_str()) == 0) return;
	}
	World w;
	if (!make_world(p, &w)) return;
	if (sim_params.verbose) printf("world_registry: registered '%s'\n", w.NAME);
	registry_worlds.push_back(w);
}

void load_world_registry(const char *registry_path) {
	registry_worlds.clear();	// replace semantics: this file is the whole table until the next load.

	FILE *fp = fopen(registry_path, "r");
	if (fp == nullptr) {
		if (sim_params.verbose) printf("world_registry: no registry file '%s' - no worlds registered\n", registry_path);
		return;
	}

	PendingWorld pending;
	char line[512];
	while (fgets(line, sizeof(line), fp) != nullptr) {
		// strip the trailing newline and any '#' comment, then walk whitespace-separated "key=value" tokens
		size_t len = strlen(line);
		while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) line[--len] = '\0';
		char *hash = strchr(line, '#');
		if (hash != nullptr) *hash = '\0';

		char *tok = strtok(line, " \t");
		while (tok != nullptr) {
			char *eq = strchr(tok, '=');
			if (eq == nullptr) { tok = strtok(nullptr, " \t"); continue; }
			*eq = '\0';
			char *key = tok;
			char *value = eq + 1;
			tok = strtok(nullptr, " \t");

			if (strcmp(key, "world") == 0) {
				register_pending(pending);	// finish the previous block first
				pending = PendingWorld();
				pending.name = value;
				continue;
			}

			if (pending.name.empty()) continue;	// settings only matter inside a world block

			if      (strcmp(key, "map") == 0)        pending.map = value;
			else if (strcmp(key, "atlas") == 0)      pending.atlas = value;
			else if (strcmp(key, "ground") == 0)     pending.ground = value;
			else if (strcmp(key, "sky") == 0)        pending.sky = value;
			else if (strcmp(key, "atlas_cols") == 0) pending.atlas_cols = atoi(value);
			else if (strcmp(key, "atlas_rows") == 0) pending.atlas_rows = atoi(value);
			else if (strcmp(key, "colors") == 0) {
				// comma-separated list of color names
				char *save = nullptr;
				for (char *cname = strtok_r(value, ",", &save); cname != nullptr; cname = strtok_r(nullptr, ",", &save))
					pending.colors.push_back(cname);
			}
			else if (strncmp(key, "tile.", 5) == 0) {
				float hue_deg = 0.0f;
				HUE_TYPE t = color_from_name(key + 5, &hue_deg);
				if (t != unknown) pending.tile[t] = atoi(value);
			}
			else if (strncmp(key, "object_texture.", 15) == 0) pending.object_texture[key + 15] = atoi(value);
			else if (strncmp(key, "object.", 7) == 0) {
				float hue_deg = 0.0f;
				HUE_TYPE t = color_from_name(key + 7, &hue_deg);
				if (t != unknown) pending.objects[t] = value;
			}
		}
	}
	register_pending(pending);	// the last block has no "world=" after it

	fclose(fp);
}

// ---------------- lookup helpers (also serve world_config's name -> World path) ----------------

bool registry_world_by_name(const char *name, World *out) {
	if (name == nullptr || out == nullptr) return false;
	for (const World &w : registry_worlds) {
		if (w.NAME != nullptr && strcmp(w.NAME, name) == 0) { *out = w; return true; }
	}
	return false;
}

const char *registry_world_name_of(const World &world) {
	return (world.NAME != nullptr) ? world.NAME : "";
}

int registry_world_count(void) {
	return (int)registry_worlds.size();
}

const char *registry_world_name_at(int i) {
	if (i < 0 || i >= (int)registry_worlds.size()) return "";
	return registry_worlds[i].NAME;
}

std::string registry_world_names(void) {
	std::string names;
	for (size_t i = 0; i < registry_worlds.size(); i++) {
		if (i > 0) names += ", ";
		names += registry_worlds[i].NAME;
	}
	return names;
}