// CREATE A MAP FROM 16 X 16 PIXEL IMAGE WITH BLACK = WALL, WHITE = NOTHING

#ifndef MAP_H
#define MAP_H

#include "raylib.h"
#include "raymath.h"
#include "data_types.hpp"

mainMap fetchMainMap(Image mazemap_img);		// create map of the Voxel World based on image (white pixel - 0 - no voxel, black pixel - 1 - voxel)
int fetchCurrChunkIdx(mainMap main_map, Vector3 point);			// fetches index of the chunk where player is currently located
chunkMap fetchChunkMap(Color *pixels, int ch_x, int ch_z, int height_px);	// map one chunk
void buildChunkMesh(mainMap *map, Mesh& mesh, int chunk_x, int chunk_z, int chunk_coord);	// build mesh of one chunk 16x16
void buildVoxelWorldMesh(mainMap *map, Mesh *mesh);		// build mesh of the Voxel World based on map

#endif
