// SELECTS PROPER COORDINATES FROM TEXTURE ATLAS TO ASSIGN CORRECT TEXTURE PER INTEGER ENCODING IT IN MAP - this files textures voxels

#include "raylib.h"
#include "raymath.h"
#include "textures.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "map.hpp"


void fetchTextureCoords(int texture_type, float *texcoords) {
	if (texture_type == red) for (int i = 0; i < 48; i++) texcoords[i] = brick[i];
	if (texture_type == green) for (int i = 0; i < 48; i++) texcoords[i] = bush[i];
	if (texture_type == pink) for (int i = 0; i < 48; i++) texcoords[i] = tree[i];
	if (texture_type == blue) for (int i = 0; i < 48; i++) texcoords[i] = building[i];
}

const float tree[48] = {
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

const float brick[48] = {
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

const float building[48] = {
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

const float bush[48] = {
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


