#ifndef WORLDS_H
#define WOLRDS_H

#include "data_types_worlds.hpp"
#include "objects.hpp"
#include "color_hue_mapping.hpp"

// PREDEFINED WORLD FOR CONVIENCE, TODO: make this possible wit proper file structure and importing and stuff 
// map defining which color maps which voxel cluster type, the colors are currently fixed to textures in textue atlas (red is top left, pink top right, green bottom left, blue bottom right)

// paths to files


inline World ConferenceWorld = {
	.MAP_IMAGE_PATH = "src/ros_voxels/core_voxels/resources/map_images/map_paper.png",
	.TEXTURE_ATLAS_PATH = "src/ros_voxels/core_voxels/resources/textures/atlas.png",
	.GROUND_TEXTURE_PATH = "src/ros_voxels/core_voxels/resources/textures/grass.png",
	.SKY_TEXTURE_PATH = "src/ros_voxels/core_voxels/resources/textures/stars.png",
	.objects = {
		{red, tree},
		{green, brick},
		{pink, building},
		{blue, bush}
	}
};

inline World SidlisteWorld = {
	.MAP_IMAGE_PATH = "src/ros_voxels/core_voxels/resources/map_images/map_sidliste.png",
	.TEXTURE_ATLAS_PATH = "src/ros_voxels/core_voxels/resources/textures/atlas.png",
	.GROUND_TEXTURE_PATH = "src/ros_voxels/core_voxels/resources/textures/grass.png",
	.SKY_TEXTURE_PATH = "src/ros_voxels/core_voxels/resources/textures/stars.png",
	.objects = {
		{red, tree},
		{green, brick},
		{pink, building},
		{blue, bush}
	}
};

inline World DesertWorld = {
	.MAP_IMAGE_PATH = "src/ros_voxels/core_voxels/resources/map_images/map_desert.png",
	.TEXTURE_ATLAS_PATH = "src/ros_voxels/core_voxels/resources/textures/desert_texture_atlas.png",
	.GROUND_TEXTURE_PATH = "src/ros_voxels/core_voxels/resources/textures/desert.png",
	.SKY_TEXTURE_PATH = "src/ros_voxels/core_voxels/resources/textures/sky_clouds.png",
	.objects = {
		{red, sand},
		{green, tree},
		{pink, rock},
		{blue, bush}
	}
};

inline World BleakCityWorld = {
	.MAP_IMAGE_PATH = "src/ros_voxels/core_voxels/resources/map_images/map_city.png",
	.TEXTURE_ATLAS_PATH = "src/ros_voxels/core_voxels/resources/textures/city_atlas.png",
	.GROUND_TEXTURE_PATH = "src/ros_voxels/core_voxels/resources/textures/asphalt.png",
	.SKY_TEXTURE_PATH = "src/ros_voxels/core_voxels/resources/textures/stars.png",
	.objects = {
		{red, fence},
		{green, brick},
		{pink, building},
		{blue, puddle}
	},
};

inline World CurrentWorld = DesertWorld;

#endif
