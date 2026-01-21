// CREATE A MAP FROM 16 X 16 PIXEL IMAGE WITH BLACK = WALL, WHITE = NOTHING

#include "raylib.h"
#include "raymath.h"
#include <stdio.h>
#include <stdlib.h>
#include "map.h"
#include "data_types.h"

const int chunk_w = 16;
const int chunk_h = 16;

// fetches index of the chunk where player is currently located
int fetchCurrChunkId(mainMap main_map) {
	int curr_chunk_id = 0;

	return curr_chunk_id;
}

//create entire map consisting of chunks
mainMap fetchMainMap(Image mazemap_img) {
	mainMap main_map = {0};
	
	Color *pixels = LoadImageColors(mazemap_img);

	

	main_map.width_px = mazemap_img.width;		// width of entire world in pixels
	main_map.height_px = mazemap_img.height;		// height of entire world in pixels
	main_map.width_chunks = (int)((main_map.width_px - 1) / 16 + 1);		// height in chunks
	main_map.height_chunks = (int)((main_map.height_px - 1)/ 16 + 1);		// width of entire world in chunks
	main_map.chunk_side = 16;		// TODO: magic number

	printf("chunk w: %d, chunk-h: %d \n", main_map.width_chunks, main_map.height_chunks);

	float arr_size = main_map.width_chunks * main_map.height_chunks * sizeof(chunkMap); 
	main_map.chunks = (chunkMap *)RL_MALLOC(arr_size);
	main_map.chunks[0].map[0] = 0;

	for (int chunk_z = 0; chunk_z < main_map.height_chunks; chunk_z++) {
		for (int chunk_x = 0; chunk_x < main_map.width_chunks; chunk_x++) {
			main_map.chunks[chunk_x + chunk_z * main_map.width_chunks] = fetchChunkMap(pixels, chunk_x, chunk_z, main_map.height_px);
		}
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
			//printf("%d ", position);
		}
		//printf("\n");
	}
	

	

	return map;
}

