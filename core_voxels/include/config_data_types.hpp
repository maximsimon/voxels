// THIS FILE DEFINES DATA TYPES USED IN config_core.hpp BECAUSE config_core.hpp VALUES ARE USED IN data_types.hpp, SO THIS FILE AVOIDS CIRCULAR INCLUSION - TODO: do this properly, this soltuion is hacky i think

#ifndef CONFIG_DATA_TYPES_H
#define CONFIG_DATA_TYPES_H

#include <vector>
#include <unordered_map>

// stuff for allowing parametrization of number of texture types and pixel color mapping to texture type (it is in in config_core.hpp)  

// pixel to hue_type mapping
enum HUE_TYPE {white, red, green, pink, blue, unknown};

struct HueClass {
    HUE_TYPE type;
    float center_hue_deg;
};

struct HueConfig {
    float white_gray_threshold = 150;
    std::vector<HueClass> colors;
};


// hue_type to voxel cluster type mapping
struct VoxelSpec {
    float x, y, z;		// local offset of voxel inside the voxel column/cluster)
    float sx, sy, sz;		// size of the voxel
};

struct ObjectDefinition {
    std::vector<VoxelSpec> voxels;
};

using ObjectMap = std::unordered_map<HUE_TYPE, ObjectDefinition>;

// fetching correct texture for voxel_cluster_type (based on hue_type of voxel)
//using TexCoordArray = std::array<float, 48>;

#endif
