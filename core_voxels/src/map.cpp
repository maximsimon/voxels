// CREATE A MAP FROM 16 X 16 PIXEL IMAGE WITH BLACK = WALL, WHITE = NOTHING

#include "raylib.h"
#include "raymath.h"
#include <stdio.h>
#include <stdlib.h>
#include "map.hpp"
#include "data_types.hpp"
#include "faces.hpp"
#include "config_core.hpp"

const int chunk_w = CHUNK_WIDTH;
const int chunk_h = CHUNK_HEIGHT;

// fetches index of the chunk in map where world_pose is currently located
int fetchCurrChunkIdx(mainMap main_map, Vector3 world_pose) {
	int curr_chunk_idx = 0;
	int chunk_x = 0;
	int chunk_z = 0;
	
	chunk_x = (int)world_pose.x / main_map.chunk_side;
	chunk_z = (int)world_pose.z / main_map.chunk_side;

	curr_chunk_idx = chunk_x + chunk_z * main_map.width_chunks;
	return curr_chunk_idx;
}

// fetches index of the cell in map where world_pose is currently located
int fetchCurrCellIdx(mainMap main_map, Vector3 world_pose) {
	int curr_cell_idx = 0;
	int cell_x = 0;
	int cell_z = 0;
	
	cell_x = (int)world_pose.x % main_map.chunk_side;
	cell_z = (int)world_pose.z % main_map.chunk_side;

	curr_cell_idx = cell_x + cell_z * main_map.chunk_side;
	return curr_cell_idx;
	
}

// fetches chunk index and index of cell inside that chunk - transforms (x,z) world positioin to (ch_idx, p_idx) map position
void fetchCurrMapCoord(mainMap &main_map, Vector3 world_pose) {
	main_map.map_coords.chunk =  fetchCurrChunkIdx(main_map, world_pose);
	main_map.map_coords.cell = fetchCurrCellIdx(main_map, world_pose);
}

//create entire map consisting of chunks
mainMap fetchMainMap(Image mazemap_img) {
	//mainMap *main_map = new mainMap();
	mainMap main_map;
	Color *pixels = LoadImageColors(mazemap_img);

	main_map.width_px = mazemap_img.width;		// width of entire world in pixels
	main_map.height_px = mazemap_img.height;		// height of entire world in pixels
	
	// check if image sides are divisible by 16 (chunk_size for now
	if (main_map.height_px % chunk_h != 0 || main_map.width_px % chunk_w != 0) {
		printf("\n\n\n ERROR: map input image has sides not divisible by %d in pixels, pls enter image with sides of multiples of %d \n\n\n", chunk_w, chunk_w);
	}

	main_map.width_chunks = (int)((main_map.width_px - 1) / chunk_w + 1);		// height in chunks
	main_map.height_chunks = (int)((main_map.height_px - 1)/ chunk_h + 1);		// width of entire world in chunks
	main_map.chunk_side = chunk_w;

	main_map.chunks = new chunkMap[main_map.width_chunks * main_map.height_chunks]();	

	printf("starting to map image pixels to binary matrix map\n");
	
	for (int chunk_z = 0; chunk_z < main_map.height_chunks; chunk_z++) {
		for (int chunk_x = 0; chunk_x < main_map.width_chunks; chunk_x++) {
			main_map.chunks[chunk_x + chunk_z * main_map.width_chunks] = fetchChunkMap(pixels, chunk_x, chunk_z, main_map.height_px);
			main_map.chunks[chunk_x + chunk_z * main_map.width_chunks].chunk_position = {(float)(chunk_x * main_map.chunk_side), 0.5f, (float)(chunk_z * main_map.chunk_side)};	// assign position (coords in the simulation) of 1st voxel in the chunk
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
	HUE_TYPE hue_type;	
	for (int z = 0; z < chunk_h; z++) {
		for (int x = 0; x < chunk_w; x++) {
			// get pixels hue
			position = (ch_z * height_px * chunk_h + z * height_px) + (ch_x * chunk_w + x);
			map.map[x + z * chunk_w] = getPixelHue(pixels[position]);	
		}
	}
	map.voxel_count = 0;
	return map;
}

// returns color (from enum variable HUE_TYPE) based on pixels real RGB/HSV color
HUE_TYPE getPixelHue(Color pixel_color) {

	if (GRAY_VALUE(pixel_color) > color_config.white_gray_threshold) return white;

	float hue = ColorToHSV(pixel_color).x;

	HUE_TYPE best_type = unknown;
	float best_distance = std::numeric_limits<float>::max();

	// find closest HUE_TYPE to pixels color
	for (const auto& color : color_config.colors) {
		float distance = std::abs(hue - color.center_hue_deg);
		distance = std::min(distance, 360.0f - distance);		// hue wraps around at 360°
		if (distance < best_distance) {
			best_distance = distance;
			best_type = color.type;
		}
	}

	return best_type;
}

// build mesh of one chunk 16x16 voxels
void buildChunkMesh(mainMap *map, Mesh& mesh, int chunk_x, int chunk_z, int chunk_coord) {
	int single_bool_coord = 0;
	int absolute_voxel_world_x = 0;
	int absolute_voxel_world_z = 0;
	for (int z = 0; z < chunk_h; z++) {
		absolute_voxel_world_z = chunk_z * map->chunk_side + z;
		for (int x = 0; x < chunk_w; x++) {
			single_bool_coord = x + z * map->chunk_side;
			absolute_voxel_world_x = chunk_x * map->chunk_side + x;
			if (map->chunks[chunk_coord].map[single_bool_coord] != white) { 
				genObject(mesh, (float)(absolute_voxel_world_x), (float)(absolute_voxel_world_z), &map->chunks[chunk_coord].voxel_count, map->chunks[chunk_coord].map[single_bool_coord]);
			}
		}
	}
	
}

// generates voxels of predefined style base on hue_type of pixel in that place in input image
void genObject(Mesh &mesh, float x, float z, int *voxel_count, int hue_type_int) {
	
	HUE_TYPE type = static_cast<HUE_TYPE>(hue_type_int);
	auto item = objects.find(type);		// find voxel cluster type from ObjectMap ('objects' are elements of ObjectMap) based on pixel hue
	
	if (item == objects.end()) return;

	const auto& object = item->second;
	
	// add voxel cluster to chunk mesh
	for (const auto& v : object.voxels) MeshVoxel(mesh, x + v.x, v.y, z + v.z, 0.0f, voxel_count, type, v.sx, v.sy, v.sz);
}

// build world based on map
void buildVoxelWorldMesh(mainMap *map, Mesh *mesh) {
	int chunk_coord = 0;
	for (int chunk_z = 0; chunk_z < map->height_chunks; chunk_z++) {
		for (int chunk_x = 0; chunk_x < map->width_chunks; chunk_x++) {
			chunk_coord = chunk_x + chunk_z * map->width_chunks;
			mesh[chunk_coord] = { 0 };		// init one element of mesh array (one 16x16 chunk)
			buildChunkMesh(map, mesh[chunk_coord], chunk_x, chunk_z, chunk_coord);		// fill one chunk mesh
		}
	}
}
