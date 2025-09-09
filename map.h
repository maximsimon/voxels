// CREATE A MAP FROM 16 X 16 PIXEL IMAGE WITH BLACK = WALL, WHITE = NOTHING

#ifndef MAP_H
#define MAP_H

#include "raylib.h"
#include "raymath.h"
#include <stdio.h>

#define GRAY_VALUE(c) ((float)(c.r + c.g + c.b)/3.0f)

typedef struct {
    int map[16 * 16];
} chunkMap;

const int chunk_w = 16;
const int chunk_h = 16;


chunkMap FetchChunkMap() {
	
	chunkMap map = {0};

	Image mazemap_img = LoadImage("mazemap.png");		// temporary, later for bigger worlds, map will be way higher up in software (above all chunks)
	Color *pixels = LoadImageColors(mazemap_img);
	
	if (mazemap_img.height != 16 || mazemap_img.width != 16) printf(" \n ERROR: mazemap_img is the wrong size \n");

	for (int z = 0; z < chunk_h; z++) {
		for (int x = 0; x < chunk_w; x++) {
			if (GRAY_VALUE(pixels[x + z * chunk_h]) < 170) {
				map.map[x + z * chunk_h] = 1;
			}
		}
	}

	UnloadImageColors(pixels);
	UnloadImage(mazemap_img);

	

	return map;
}

#endif
