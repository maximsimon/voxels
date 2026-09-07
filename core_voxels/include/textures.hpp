#ifndef TEXTURES_H
#define TEXTURES_H

#include "data_types.hpp"

void fetchTextureCoords(HUE_TYPE texture_type, float *texcoords);

extern const float first_tex_coords[48];
extern const float second_tex_coords[48];
extern const float third_tex_coords[48];
extern const float fourth_tex_coords[48];

#endif
