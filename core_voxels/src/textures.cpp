// SELECTS PROPER COORDINATES FROM TEXTURE ATLAS TO ASSIGN CORRECT TEXTURE PER INTEGER ENCODING IT IN MAP - this files textures voxels

#include "raylib.h"
#include "raymath.h"
#include "textures.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "map.hpp"
#include "config_core.hpp"
#include "config_data_types.hpp"

//TODO: parametrize pickign texture from texture atlas - you need to automate the 48 values in the tex_corrds array

void fetchTextureCoords(HUE_TYPE texture_type, float *texcoords) {
	if (texture_type == red) for (int i = 0; i < 48; i++) texcoords[i] = first_tex_coords[i];
	if (texture_type == green) for (int i = 0; i < 48; i++) texcoords[i] = second_tex_coords[i];
	if (texture_type == pink) for (int i = 0; i < 48; i++) texcoords[i] = third_tex_coords[i];
	if (texture_type == blue) for (int i = 0; i < 48; i++) texcoords[i] = fourth_tex_coords[i];
}

// TODO: on 1 side of voxels the texture is rotated by 90 degrees, figure out properly how the texture coords work

const float first_tex_coords[48] = {
	0.0f, 0.0f,
	0.5f, 0.0f,
	0.5f, 0.5f,
	0.0f, 0.5f,
	0.5f, 0.0f,
	0.5f, 0.5f,
	0.0f, 0.5f,
	0.0f, 0.0f,
	0.0f, 0.5f,
	0.0f, 0.0f,
	0.5f, 0.0f,
	0.5f, 0.5f,
	0.5f, 0.5f,
	0.0f, 0.5f,
	0.0f, 0.0f,
	0.5f, 0.0f,
	0.5f, 0.0f,
	0.5f, 0.5f,
	0.0f, 0.5f,
	0.0f, 0.0f,
	0.0f, 0.0f,
	0.5f, 0.0f,
	0.5f, 0.5f,
	0.0f, 0.5f
};

const float second_tex_coords[48] = {
	0.5f, 0.0f,
	1.0f, 0.0f,
	1.0f, 0.5f,
	0.5f, 0.5f,
	0.5f, 0.0f,
	1.0f, 0.0f,
	1.0f, 0.5f,
	0.5f, 0.5f,
	0.5f, 0.0f,
	1.0f, 0.0f,
	1.0f, 0.5f,
	0.5f, 0.5f,
	0.5f, 0.0f,
	1.0f, 0.0f,
	1.0f, 0.5f,
	0.5f, 0.5f,
	0.5f, 0.0f,
	1.0f, 0.0f,
	1.0f, 0.5f,
	0.5f, 0.5f,
	0.5f, 0.0f,
	1.0f, 0.0f,
	1.0f, 0.5f,
	0.5f, 0.5f,
};

const float third_tex_coords[48] = {
	0.0f, 0.5f,
	0.5f, 0.5f,
	0.5f, 1.0f,
	0.0f, 1.0f,
	0.0f, 0.5f,
	0.5f, 0.5f,
	0.5f, 1.0f,
	0.0f, 1.0f,
	0.0f, 0.5f,
	0.5f, 0.5f,
	0.5f, 1.0f,
	0.0f, 1.0f,
	0.0f, 0.5f,
	0.5f, 0.5f,
	0.5f, 1.0f,
	0.0f, 1.0f,
	0.0f, 0.5f,
	0.5f, 0.5f,
	0.5f, 1.0f,
	0.0f, 1.0f,
	0.0f, 0.5f,
	0.5f, 0.5f,
	0.5f, 1.0f,
	0.0f, 1.0f,
};

const float fourth_tex_coords[48] = {
	0.5f, 0.5f,
	1.0f, 0.5f,
	1.0f, 1.0f,
	0.5f, 1.0f,
	1.0f, 0.5f,
	1.0f, 1.0f,
	0.5f, 1.0f,
	0.5f, 0.5f,
	0.5f, 1.0f,
	0.5f, 0.5f,
	1.0f, 0.5f,
	1.0f, 1.0f,
	1.0f, 1.0f,
	0.5f, 1.0f,
	0.5f, 0.5f,
	1.0f, 0.5f,
	1.0f, 0.5f,
	1.0f, 1.0f,
	0.5f, 1.0f,
	0.5f, 0.5f,
	0.5f, 0.5f,
	1.0f, 0.5f,
	1.0f, 1.0f,
	0.5f, 1.0f
};


