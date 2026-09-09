// BUILDING / TEARING DOWN / RELOADING THE WORLD
//
// build_world() is the single place the world geometry is created - init_sim(), the
// live-reload key and reset(world, ...) all go through it, so they cannot drift apart.
// destroy_world() is its exact inverse: without it every world swap would leak a full
// set of chunk meshes, models and textures on the GPU, which matters as soon as an RL
// loop starts resetting between episodes.

#include "world_loading.hpp"
#include "data_types.hpp"
#include "map.hpp"
#include "world_config.hpp"
#include "small_handy_stuff.hpp"
#include "sim_params.hpp"
#include "rlgl.h"
#include <cstring>
#include <iostream>

// print only when the caller asked for chatter - the world build is on the reset path
static void world_log(const char *msg) {
	if (sim_params.verbose) printf("%s", msg);
}

void build_world(VoxelWorld *vw) {
	const World &world = vw->current_world;

	// BUILD MAP AND THE VOXEL WORLD (its appearance and "physical" structure)
	Image mazemap_image = LoadImage(world.MAP_IMAGE_PATH);	//TODO: add some error handling and printing if file does not load
	mainMap main_map = fetchMainMap(mazemap_image);		// get map of voxel world (1 - voxel, 0 - no voxel)
	world_log("map built succesffully\n");

	const int chunk_count = main_map.width_chunks * main_map.height_chunks;

	world_log("initilizing mesh\n");
	Mesh *maze_mesh = new Mesh[chunk_count]();
	world_log("mesh initilized\n");

	world_log("attempting to build maze mesh\n");
	buildVoxelWorldMesh(&main_map, maze_mesh);	// build world based on map
	world_log("maze mesh built successfully\n");

	// maze model
	Model *model = new Model[chunk_count]();
	Texture2D texture = LoadTexture(world.TEXTURE_ATLAS_PATH);    // Load texture atlas
	for (int i = 0; i < chunk_count; i++) {
		UploadMesh(&maze_mesh[i], false);				// upload world
		model[i] = LoadModelFromMesh(maze_mesh[i]);                  // Load model from generated mesh
		model[i].materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texture;    // Set map diffuse texture
	}

	vw->main_map = main_map;
	vw->maze_mesh = maze_mesh;
	vw->maze_model = model;
	vw->maze_chunk_count = chunk_count;
	vw->atlas_texture = texture;

	// mesh for the ground
	Texture2D groundTex = LoadTexture(world.GROUND_TEXTURE_PATH);
	SetTextureFilter(groundTex, TEXTURE_FILTER_POINT);
	//SetTextureWrap(groundTex, TEXTURE_WRAP_REPEAT); // important for tiling

	Mesh ground_mesh = GenMeshPlane(500, 500, 10, 20);	// GenMesh* already uploads to the GPU; a second UploadMesh here just warned about re-loading
	Model ground_model = LoadModelFromMesh(ground_mesh);

	ground_model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = groundTex;
	vw->ground_model = ground_model;
	vw->ground_texture = groundTex;

	// mesh and model of the sky
	Mesh sky_mesh = GenMeshHemiSphere(500.0f, 32, 32);
	Model sky_model = LoadModelFromMesh(sky_mesh);
	Texture2D sky_texture = LoadTexture(world.SKY_TEXTURE_PATH);    // Load map texture
	sky_model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = sky_texture;
	SetTextureWrap(sky_texture, TEXTURE_WRAP_REPEAT); // important for tiling
	sky_model.transform = MatrixScale(1, 1, -1);
	vw->sky_model = sky_model;
	vw->sky_texture = sky_texture;

	vw->world_built = true;
	// END OF BUILDING THE VOXEL WORLD (appearance and "physical" structure)
}

// point a model's diffuse map back at raylib's default texture, so UnloadModel's
// material cleanup does not try to delete a texture we own and unload separately
// (the atlas is shared by every chunk model - it must be deleted exactly once).
static void detach_diffuse_texture(Model *model) {
	for (int m = 0; m < model->materialCount; m++) {
		model->materials[m].maps[MATERIAL_MAP_DIFFUSE].texture.id = rlGetTextureIdDefault();
	}
}

void destroy_world(VoxelWorld *vw) {
	if (!vw->world_built) return;

	for (int i = 0; i < vw->maze_chunk_count; i++) {
		detach_diffuse_texture(&vw->maze_model[i]);
		UnloadModel(vw->maze_model[i]);		// also unloads the chunk mesh it took ownership of
	}
	delete[] vw->maze_model;
	delete[] vw->maze_mesh;			// the Mesh structs themselves; their buffers went with UnloadModel
	vw->maze_model = nullptr;
	vw->maze_mesh = nullptr;
	vw->maze_chunk_count = 0;

	detach_diffuse_texture(&vw->ground_model);
	UnloadModel(vw->ground_model);
	detach_diffuse_texture(&vw->sky_model);
	UnloadModel(vw->sky_model);

	UnloadTexture(vw->atlas_texture);
	UnloadTexture(vw->ground_texture);
	UnloadTexture(vw->sky_texture);

	delete[] vw->main_map.chunks;
	vw->main_map.chunks = nullptr;

	vw->world_built = false;
}

void reload_world(VoxelWorld *vw) {
	world_log("\n\n R       E        L        O        A        D        I        N        G                 W        O        R        L        D \n\n");

	load_world_config("core_voxels/resources/worlds/worlds.config");
	vw->current_world = CurrentWorld;

	if (sim_params.verbose) printWorld(CurrentWorld);

	destroy_world(vw);
	build_world(vw);

	world_log(" \n\n R        E        L        O        A        D        E        D \n\n");
}

bool switch_world(VoxelWorld *vw, const char *world_name) {
	World requested;
	if (!world_by_name(world_name, &requested)) return false;

	// same world already standing - nothing to rebuild, keep the reset path cheap
	if (vw->world_built && strcmp(requested.MAP_IMAGE_PATH, vw->current_world.MAP_IMAGE_PATH) == 0
	    && strcmp(requested.TEXTURE_ATLAS_PATH, vw->current_world.TEXTURE_ATLAS_PATH) == 0
	    && strcmp(requested.GROUND_TEXTURE_PATH, vw->current_world.GROUND_TEXTURE_PATH) == 0
	    && strcmp(requested.SKY_TEXTURE_PATH, vw->current_world.SKY_TEXTURE_PATH) == 0) {
		return true;
	}

	CurrentWorld = requested;
	vw->current_world = requested;
	destroy_world(vw);
	build_world(vw);
	return true;
}
