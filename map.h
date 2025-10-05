// CREATE A MAP FROM 16 X 16 PIXEL IMAGE WITH BLACK = WALL, WHITE = NOTHING

#ifndef MAP_H
#define MAP_H

#include "raylib.h"
#include "raymath.h"
#include <stdio.h>
#include <stdlib.h>

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

const int chunk_w = 16;
const int chunk_h = 16;

mainMap fetchMainMap();
int fetchCurrChunkId(mainMap main_map);			// fetches index of the chunk where player is currently located
chunkMap fetchChunkMap(Color *piels, int ch_x, int ch_z, int height_px);	// map one chunk

// fetches index of the chunk where player is currently located
int fetchCurrChunkId(mainMap main_map) {
	int curr_chunk_id = 0;

	

	return curr_chunk_id;

}

//create entire map consisting of chunks
mainMap fetchMainMap() {
	mainMap main_map = {0};
	
	Image mazemap_img = LoadImage("mazemap_large.png");		// temporary, later for bigger worlds, map will be way higher up in software (above all chunks)
	Color *pixels = LoadImageColors(mazemap_img);

	

	main_map.width_px = mazemap_img.width;		// width of entire world in pixels
	main_map.height_px = mazemap_img.height;		// height of entire world in pixels
	main_map.width_chunks = (int)(main_map.width_px / 16);		// height in chunks
	main_map.height_chunks = (int)(main_map.height_px / 16);		// width of entire world in chunks
	main_map.chunk_side = 16;		// TODO: magic number

	printf("chunk w: %d, chunk-h: %d \n", main_map.width_chunks, main_map.height_chunks);

	float arr_size = main_map.width_chunks * main_map.height_chunks * sizeof(chunkMap); 
	main_map.chunks = (chunkMap *)RL_MALLOC(arr_size);
	main_map.chunks[0].map[0] = 0;

	for (int chunk_z = 0; chunk_z < main_map.height_chunks; chunk_z++) {
		for (int chunk_x = 0; chunk_x < main_map.width_chunks; chunk_x++) {
			main_map.chunks[chunk_x + chunk_z * main_map.width_chunks] = fetchChunkMap(pixels, chunk_x, chunk_z, main_map.height_px);
		}
		printf("\n");
	}


	UnloadImageColors(pixels);
	UnloadImage(mazemap_img);

	return main_map;
}

//create map of one chunk
chunkMap fetchChunkMap(Color *pixels, int ch_x, int ch_z, int height_px) {
	
	chunkMap map = {0};

	int position = 0;
	
	for (int z = 0; z < chunk_h; z++) {
		for (int x = 0; x < chunk_w; x++) {
			position = (ch_z * height_px * chunk_h + z * height_px) + (ch_x * chunk_w + x);
			if (GRAY_VALUE(pixels[position]) < 170) {
				map.map[x + z * chunk_w] = 1;
			}
			printf("%d ", position);
		}
		printf("\n");
	}
	

	

	return map;
}

#endif
