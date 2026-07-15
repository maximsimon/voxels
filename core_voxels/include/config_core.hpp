#ifndef CONFIG_CORE_H
#define CONFIG_CORE_H

#include "config_data_types.hpp"

// simulation window size
inline const int SCREEN_WIDTH = 1600;
inline const int SCREEN_HEIGHT = 850;

// sped of movement and rotation
inline const float SPEED = 0.05f;	
inline const float TURN_SPEED = 0.01f;

inline const float SPEED_GOD = 0.5f;
inline const float TURN_SPEED_GOD = 0.05f;

// lidar constants
inline const int NUM_LIDAR_RAYS = 180;
inline const float MAX_LIDAR_RANGE = 40.0f;

// STUFF FOR MAP.HPP AND BUILDING THE WORLD IN GENERAL


// world dimension
inline const int CHUNK_WIDTH = 16;
inline const int CHUNK_HEIGHT = 16;

// textures - color definitions
// IMPORTANT NOTE: if you edit number of colors you have to edit it in map.hpp and map.cpp as well (in the beginning of the file)		// TODO: fix this 
inline float RED_HUE = 0.0f;
inline float GREEN_HUE = 120.0f;
inline float BLUE_HUE = 240.0f;
inline float PINK_HUE = 320.0f;
	
inline HueConfig color_config = {150.0f, {{red,  RED_HUE}, {green, GREEN_HUE},	{blue,  BLUE_HUE}, {pink,  PINK_HUE}}};	

// voxel clusters types - each vector is {voxel local offset, size}, e.g. tree is 3 voxels above each other each with width and length 0.5 and height 1.0
inline ObjectDefinition tree = {
    {
        {0, 0, 0,	0.5f, 1.0f, 0.5f},
        {0, 1, 0,	0.5f, 1.0f, 0.5f},
        {0, 2, 0,	0.5f, 1.0f, 0.5f},
    }
};

inline ObjectDefinition brick = {
    {
        {0, 0, 0,	 1.0f, 2.0f, 1.0f},
    }
};

inline ObjectDefinition bush = {
    {
        {0, 0, 0,	 1.0f, 1.0f, 1.0f},
    }
};

inline ObjectDefinition building = {
    {
        {0, 0, 0,	 1.0f, 2.0f, 1.0f},
        {0, 1, 0,	 1.0f, 2.0f, 1.0f},
        {0, 2, 0,	 1.0f, 2.0f, 1.0f},
        {0, 3, 0,	 1.0f, 2.0f, 1.0f},
    }
};

inline ObjectDefinition rock = {
    {
        {0, 0, 0,	 1.5f, 1.5f, 1.5f},
    }
};

inline ObjectDefinition sand = {
    {
        {0, -0.5f, 0,	 1.0f, 0.5f, 1.0f},
    }
};

inline ObjectDefinition fence = {
    {
        {0, 0, 0,	 0.1f, 1.0f, 1.0f},
    }
};

inline ObjectDefinition puddle = {
    {
        {0, -0.9, 0,	 1.0f, 1.0f, 1.0f},
    }
};

// PREDEFINED WORLD FOR CONVIENCE, TODO: make this possible wit proper file structure and importing and stuff 
// map defining which color maps which voxel cluster type, the colors are currently fixed to textures in textue atlas (red is top left, pink top right, green bottom left, blue bottom right)

// paths to files

// PAPER (VTR testbed default)
inline const char* MAP_IMAGE_PATH = "src/ros_voxels/core_voxels/resources/map_images/map_paper.png";
inline const char* TEXTURE_ATLAS_PATH = "src/ros_voxels/core_voxels/resources/textures/atlas.png";
inline const char* GROUND_TEXTURE_PATH = "src/ros_voxels/core_voxels/resources/textures/grass.png";
inline const char* SKY_TEXTURE_PATH = "src/ros_voxels/core_voxels/resources/textures/stars.png";
inline ObjectMap objects = {
	{red, tree},
	{green, brick},
	{pink, building},
	{blue, bush}
};

// SIDLISTE
/*
inline const char* MAP_IMAGE_PATH = "src/ros_voxels/core_voxels/resources/map_images/map_sidliste.png";
inline const char* TEXTURE_ATLAS_PATH = "src/ros_voxels/core_voxels/resources/textures/atlas.png";
inline const char* GROUND_TEXTURE_PATH = "src/ros_voxels/core_voxels/resources/textures/grass.png";
inline const char* SKY_TEXTURE_PATH = "src/ros_voxels/core_voxels/resources/textures/stars.png";
inline ObjectMap objects = {
	{red, tree},
	{green, brick},
	{pink, building},
	{blue, bush}
};
*/

// DESERT
/*
inline const char* MAP_IMAGE_PATH = "src/ros_voxels/core_voxels/resources/map_images/map_desert.png";
inline const char* TEXTURE_ATLAS_PATH = "src/ros_voxels/core_voxels/resources/textures/desert_texture_atlas.png";
inline const char* GROUND_TEXTURE_PATH = "src/ros_voxels/core_voxels/resources/textures/desert.png";
inline const char* SKY_TEXTURE_PATH = "src/ros_voxels/core_voxels/resources/textures/sky_clouds.png";
inline ObjectMap objects = {
	{red, sand},
	{green, tree},
	{pink, rock},
	{blue, bush}
};
*/

// BLEAK CITY, with puddles
/*
inline const char* MAP_IMAGE_PATH = "src/ros_voxels/core_voxels/resources/map_images/map_city.png";
inline const char* TEXTURE_ATLAS_PATH = "src/ros_voxels/core_voxels/resources/textures/city_atlas.png";
inline const char* GROUND_TEXTURE_PATH = "src/ros_voxels/core_voxels/resources/textures/asphalt.png";
inline const char* SKY_TEXTURE_PATH = "src/ros_voxels/core_voxels/resources/textures/stars.png";

inline ObjectMap objects = {
	{red, fence},
	{green, brick},
	{pink, building},
	{blue, puddle}
};
*/

#endif 

