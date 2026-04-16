// SELECTS PROPER COORDINATES FROM TEXTURE ATLAS TO ASSIGN CORRECT TEXTURE PER INTEGER ENCODING IT IN MAP - this files textures voxels

#include "raylib.h"
#include "raymath.h"
#include "textures.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void fetchTextureCoords(int texture_type, float *texcoords) {
	if (texture_type == 1) for (int i = 0; i < 48; i++) texcoords[i] = steel[i];
	if (texture_type == 2) for (int i = 0; i < 48; i++) texcoords[i] = bark[i];
	if (texture_type == 3) for (int i = 0; i < 48; i++) texcoords[i] = orange[i];
	if (texture_type == 4) for (int i = 0; i < 48; i++) texcoords[i] = blue[i];
}

const float steel[48] = {
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

const float bark[48] = {
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

const float orange[48] = {
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

const float blue[48] = {
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


