// SELECTS PROPER COORDINATES FROM TEXTURE ATLAS TO ASSIGN CORRECT TEXTURE PER INTEGER ENCODING IT IN MAP - this files textures voxels

#include "raylib.h"
#include "raymath.h"
#include "textures.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "map.hpp"
#include "config_core.hpp"
#include "data_types.hpp"

// fill texcoords (48 floats = 6 faces x 4 vertices x 2 UV values) with the quad of atlas tile `tile_index`; row/col come from the world's atlas_cols x atlas_rows grid, so any tile count works as long as the atlas image sits on that grid
void fetchTextureCoords(int tile_index, float *texcoords) {
	const int cols = CurrentWorld.atlas_cols;
	const int rows = CurrentWorld.atlas_rows;
	if (cols <= 0 || rows <= 0) {
		for (int i = 0; i < 48; i++) texcoords[i] = 0.0f;
		return;
	}

	const int tile_x = tile_index % cols;
	const int tile_z = tile_index / cols;
	const float u0 = (float)tile_x / cols;
	const float v0 = (float)tile_z / rows;
	const float u1 = u0 + 1.0f / cols;
	const float v1 = v0 + 1.0f / rows;

	// one uniform quad on all 6 faces (the old hand-written arrays per hue are gone)
	for (int face = 0; face < 6; face++) {
		float *tc = texcoords + face * 8;
		tc[0] = u0; tc[1] = v0;
		tc[2] = u1; tc[3] = v0;
		tc[4] = u1; tc[5] = v1;
		tc[6] = u0; tc[7] = v1;
	}
}