// CREATE A MAP FROM 16 X 16 PIXEL IMAGE WITH BLACK = WALL, WHITE = NOTHING

#ifndef MAP_H
#define MAP_H

#include "raylib.h"
#include "raymath.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GRAY_VALUE(c) ((float)(c.r + c.g + c.b)/3.0f)

typedef struct {
    int map[16 * 16];
} chunkMap;

typedef struct {
	chunkMap *chunks;
	int width_px;		// width of entire world in pixels
	int height_px;		// height of entire world in pixels
	int width_chunks; 		// height in chunks
	int height_chunks;		// width of entire world in chunks
	int chunk_side;			// size one side of chunk in pixels (chunks are squares)
} mainMap;

extern const int chunk_w;
extern const int chunk_h;

mainMap fetchMainMap(Image mazemap_img);
int fetchCurrChunkId(mainMap main_map);			// fetches index of the chunk where player is currently located
chunkMap fetchChunkMap(Color *pixels, int ch_x, int ch_z, int height_px);	// map one chunk


#ifdef __cplusplus
}
#endif

#endif
