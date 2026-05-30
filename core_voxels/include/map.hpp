// CREATE A MAP FROM 16 X 16 PIXEL IMAGE WITH BLACK = WALL, WHITE = NOTHING

#ifndef MAP_H
#define MAP_H

#include "raylib.h"
#include "raymath.h"
#include "data_types.hpp"

enum HUE_TYPE {white, red, green, pink, blue, unknown};

mainMap fetchMainMap(Image mazemap_img);		// create map of the Voxel World based on image (white pixel - 0 - no voxel, black pixel - 1 - voxel)
chunkMap fetchChunkMap(Color *pixels, int ch_x, int ch_z, int height_px);	// map one chunk
HUE_TYPE getPixelHue(Color pixel);			// get hue of a pixel
void buildChunkMesh(mainMap *map, Mesh &mesh, int chunk_x, int chunk_z, int chunk_coord);	// build mesh of one chunk 16x16
void buildVoxelWorldMesh(mainMap *map, Mesh *mesh);		// build mesh of the Voxel World based on map
void genObject(Mesh &mesh, float absolute_voxel_world_x, float absolute_voxel_world_z, int *voxel_count, int hue_type);
int fetchCurrChunkIdx(mainMap main_map, Vector3 point);			// fetches index of the chunk where player is currently located
void fetchCurrMapCoord(mainMap &main_map, Vector3 point);	// fetches chunk index and index of cell inside that chunk - transforms (x,z) world positioin to (ch_idx, p_idx) map position


#endif
