#ifndef DATA_TYPES_WORLDS_H
#define DATA_TYPES_WORLDS_H

#include <unordered_map>
#include <vector>

// stuff for allowing parametrization of number of texture types and pixel color mapping to texture type (it is in in config_core.hpp)  

// pixel to hue_type mapping
enum HUE_TYPE {white, red, green, pink, blue, unknown};

typedef struct {
	HUE_TYPE type;
	float center_hue_deg;
} HueClass;

typedef struct {
	float white_gray_threshold = 150;
	std::vector<HueClass> colors;
} HueConfig; 

// hue_type to voxel cluster type mapping
typedef struct {
	float x, y, z;		// local offset of voxel inside the voxel column/cluster)
	float sx, sy, sz;		// size of the voxel
} VoxelSpec ;

typedef struct {
	std::vector<VoxelSpec> voxels;
} ObjectDefinition ;

// fetching correct texture for voxel_cluster_type (based on hue_type of voxel)
using ObjectMap = std::unordered_map<HUE_TYPE, ObjectDefinition>;	//using TexCoordArray = std::array<float, 48>;

// complete definition of a World
typedef struct {
	const char* MAP_IMAGE_PATH;
	const char* TEXTURE_ATLAS_PATH;
	const char* GROUND_TEXTURE_PATH;
	const char* SKY_TEXTURE_PATH;
	ObjectMap objects;
} World;


#endif
