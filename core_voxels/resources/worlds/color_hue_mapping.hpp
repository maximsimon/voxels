#ifndef COLOR_HUE_MAPPING
#define COLOR_HUE_MAPPING

#include "data_types_worlds.hpp"

// textures - color definitions
// IMPORTANT NOTE: if you edit number of colors you have to edit it in map.hpp and map.cpp as well (in the beginning of the file)		// TODO: fix this 
inline float RED_HUE = 0.0f;
inline float GREEN_HUE = 120.0f;
inline float BLUE_HUE = 240.0f;
inline float PINK_HUE = 320.0f;
	
inline HueConfig color_config = {150.0f, {{red,  RED_HUE}, {green, GREEN_HUE},	{blue,  BLUE_HUE}, {pink,  PINK_HUE}}};	


#endif
