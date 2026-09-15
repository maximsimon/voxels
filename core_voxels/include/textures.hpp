#ifndef TEXTURES_H
#define TEXTURES_H

#include "data_types.hpp"

void fetchTextureCoords(int tile_index, float *texcoords);		// fills 48 floats (6 faces x 4 verts x 2 UV) with the quad of atlas tile `tile_index` from the world's atlas grid

#endif
