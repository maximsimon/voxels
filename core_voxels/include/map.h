// CREATE A MAP FROM 16 X 16 PIXEL IMAGE WITH BLACK = WALL, WHITE = NOTHING

#ifndef MAP_H
#define MAP_H

#include "raylib.h"
#include "raymath.h"
#include "data_types.h"

#ifdef __cplusplus
extern "C" {
#endif

mainMap fetchMainMap(Image mazemap_img);
int fetchCurrChunkId(mainMap main_map);			// fetches index of the chunk where player is currently located
chunkMap fetchChunkMap(Color *pixels, int ch_x, int ch_z, int height_px);	// map one chunk

#ifdef __cplusplus
}
#endif

#endif
