#ifndef COLOR_HUE_MAPPING
#define COLOR_HUE_MAPPING

#include "data_types_worlds.hpp"
#include <cstring>

// textures - color definitions: the hue center each named pixel color is matched against.
// the set of colors a world actually uses, and what each means there, lives in the world
// (hue_config + objects + hue_tile), so this is just the shared default palette the
// predefined worlds and getPixelHue fall back to.
inline float WHITE_HUE   = 0.0f;
inline float RED_HUE    = 0.0f;
inline float ORANGE_HUE = 30.0f;
inline float YELLOW_HUE = 60.0f;
inline float GREEN_HUE  = 120.0f;
inline float CYAN_HUE   = 180.0f;
inline float BLUE_HUE   = 240.0f;
inline float PURPLE_HUE = 270.0f;
inline float PINK_HUE   = 320.0f;
inline float BROWN_HUE  = 30.0f;

// build the full 8-color palette as a HueConfig; skip brown (it would catch dark gray
// pixels whose hue comes back as ~0).  hue_tolerance_deg = 45 keeps every existing map
// pixel matching (their furthest hue sits 41.5 deg from its center) while dropping hues
// that clearly belong to no color.
inline HueConfig default_color_config() {
	HueConfig cfg;
	cfg.white_gray_threshold = 150.0f;
	cfg.hue_tolerance_deg = 45.0f;
	cfg.colors = {
		{red,    RED_HUE},
		{orange, ORANGE_HUE},
		{yellow, YELLOW_HUE},
		{green,  GREEN_HUE},
		{cyan,   CYAN_HUE},
		{blue,   BLUE_HUE},
		{purple, PURPLE_HUE},
		{pink,   PINK_HUE},
	};
	return cfg;
}

// the palette the predefined worlds' map images were drawn with.  keeping it to just these
// four colors matters: their green pixels sit at ~79 deg, which is nearer yellow (60) than
// green (120), so a full palette would silently reclassify them as yellow objects.
inline HueConfig legacy_color_config() {
	HueConfig cfg = default_color_config();
	cfg.colors = {
		{red,   RED_HUE},
		{green, GREEN_HUE},
		{blue,  BLUE_HUE},
		{pink,  PINK_HUE},
	};
	return cfg;
}

// the classic factory-default palette - predefined worlds copy it into their hue_config,
// getPixelHue falls back to it when a world defines no colors of its own.
inline HueConfig color_config = legacy_color_config();	

// the default hue a HUE_TYPE matches to; used by registry worlds that only name their colors.
inline float color_center_of(HUE_TYPE type) {
	switch (type) {
		case white:  return WHITE_HUE;
		case red:    return RED_HUE;
		case orange: return ORANGE_HUE;
		case yellow: return YELLOW_HUE;
		case green:  return GREEN_HUE;
		case cyan:   return CYAN_HUE;
		case blue:   return BLUE_HUE;
		case purple: return PURPLE_HUE;
		case pink:   return PINK_HUE;
		case brown:  return BROWN_HUE;
		default:     return 0.0f;
	}
}

// parse a registry color name into a HUE_TYPE; unknown comes back for anything else.
inline HUE_TYPE hue_by_name(const char *name) {
	if (strcmp(name, "white") == 0)  return white;
	if (strcmp(name, "red") == 0)    return red;
	if (strcmp(name, "orange") == 0) return orange;
	if (strcmp(name, "yellow") == 0) return yellow;
	if (strcmp(name, "green") == 0)  return green;
	if (strcmp(name, "cyan") == 0)   return cyan;
	if (strcmp(name, "blue") == 0)   return blue;
	if (strcmp(name, "purple") == 0) return purple;
	if (strcmp(name, "pink") == 0)   return pink;
	if (strcmp(name, "brown") == 0)  return brown;
	return unknown;
}


#endif
