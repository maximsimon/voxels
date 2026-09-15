#ifndef DATA_TYPES_WORLDS_H
#define DATA_TYPES_WORLDS_H

#include <unordered_map>
#include <vector>

// stuff for allowing parametrization of number of texture types and pixel color mapping to texture type (it is in in config_core.hpp)  

// pixel to hue_type mapping - the named colors are the palette every map image can draw with; which of them are active in a world and what they mean there is decided by that world's hue_config + objects map (white = empty cell, unknown = no color matched)
enum HUE_TYPE {white, red, orange, yellow, green, cyan, blue, pink, purple, brown, unknown};

typedef struct {
	HUE_TYPE type;
	float center_hue_deg;
} HueClass;

typedef struct {
	float white_gray_threshold = 150;		// pixels brighter than this (average RGB) count as empty/white
	float hue_tolerance_deg = 45;			// pixels whose hue is further than this from every center are dropped (unknown)
	std::vector<HueClass> colors;			// the named colors the world's map image actually uses
} HueConfig; 

// hue_type to voxel cluster type mapping
typedef struct {
	float x, y, z;		// local offset of voxel inside the voxel column/cluster)
	float sx, sy, sz;		// size of the voxel
} VoxelSpec ;

typedef struct {
	std::vector<VoxelSpec> voxels;
	int texture = -1;			// which tile of the world's atlas this object renders with; -1 = fall back to the world's hue_tile mapping
} ObjectDefinition ;

// fetching correct texture for voxel_cluster_type (based on hue_type of voxel)
using ObjectMap = std::unordered_map<HUE_TYPE, ObjectDefinition>;	//using TexCoordArray = std::array<float, 48>;

// complete definition of a World
typedef struct {
	const char* NAME;			// unique name, used by Reset(world, ...) and the registry - empty until the world is registered
	const char* MAP_IMAGE_PATH;
	const char* TEXTURE_ATLAS_PATH;
	const char* GROUND_TEXTURE_PATH;
	const char* SKY_TEXTURE_PATH;
	int atlas_cols = 2;			// number of texture tiles per row in the atlas image
	int atlas_rows = 2;			// number of texture tiles per column in the atlas image
	HueConfig hue_config;			// the pixel colors this world's map uses and their hue centers
	ObjectMap objects;			// hue_type -> voxel cluster type
	std::unordered_map<HUE_TYPE, int> hue_tile;	// fallback: hue_type -> atlas tile index, used only when the object has texture == -1

	// resolved tile index for a hue - object textures win, otherwise the world's hue_tile
	int textureFor(HUE_TYPE type) const {
		auto it = hue_tile.find(type);
		return (it == hue_tile.end()) ? 0 : it->second;
	}
} World;

// the currently active world - set by load_world_config / switch_world before any build,
// read by map/texture building.  worlds are defined in worlds.registry, not in code.
inline World CurrentWorld = {};


#endif
