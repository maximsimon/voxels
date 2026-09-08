// RELOAD WORLD - FOR LIVE CHANGING OF WORLD APPEARANCE

#include "world_loading.hpp"
#include "data_types.hpp"
#include "map.hpp"
#include "world_config.hpp"
#include "small_handy_stuff.hpp"
#include <iostream>

void reload_world(VoxelWorld *vw) {
	// reload world

	printf("\n\n R       E        L        O        A        D        I        N        G                 W        O        R        L        D \n\n"); 
	
	load_world_config("core_voxels/resources/worlds/worlds.config");
	vw->current_world = CurrentWorld;

	printWorld(CurrentWorld);
	//mby add some check like - if nothing changed dont reload, or if only ground or sky changed dont reload world mesh and model ...

	// BUILD MAP AND THE VOXEL WORLD world (it's appearance and "physical" strcture")	
	Image mazemap_image = LoadImage(CurrentWorld.MAP_IMAGE_PATH);	//TODO: add some error handling and printing if file does not load
	mainMap main_map = fetchMainMap(mazemap_image);		// get map of voxel world (1 - voxel, 0 - no voxel)
	printf("map built succesffully\n");
	
	printf("initilizing mesh\n");
	Mesh *maze_mesh = new Mesh[main_map.width_chunks * main_map.height_chunks]();
	printf("mesh initilized\n");

	printf("attempting to build maze mesh\n");
	buildVoxelWorldMesh(&main_map, maze_mesh);	// build world based on map
	printf("maze mesh built successfully\n");
	
	// maze model
	Model *model = new Model[main_map.width_chunks * main_map.height_chunks]();
	Texture2D texture = LoadTexture(CurrentWorld.TEXTURE_ATLAS_PATH);    // Load texture atlas
	for (int i = 0; i < main_map.width_chunks * main_map.height_chunks; i++) {
		UploadMesh(&maze_mesh[i], false);				// upload world
		model[i] = LoadModelFromMesh(maze_mesh[i]);                  // Load model from generated mesh
		model[i].materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texture;    // Set map diffuse texture
	}
		
	vw->main_map = main_map;
	vw->maze_mesh = maze_mesh;
	vw->maze_model = model;

	//printMap(main_map);
	
	// mesh for the ground
	Texture2D groundTex = LoadTexture(CurrentWorld.GROUND_TEXTURE_PATH);
	SetTextureFilter(groundTex, TEXTURE_FILTER_POINT);
	//SetTextureWrap(groundTex, TEXTURE_WRAP_REPEAT); // important for tiling
	
	Mesh ground_mesh = GenMeshPlane(500, 500, 10, 20);
	UploadMesh(&ground_mesh, false);
	Model ground_model = LoadModelFromMesh(ground_mesh);

	ground_model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = groundTex;
	vw->ground_model = ground_model;	
	
	// mesh and model of the sky
	Mesh sky_mesh = GenMeshHemiSphere(500.0f, 32, 32);
	Model sky_model = LoadModelFromMesh(sky_mesh);
	Texture2D sky_texture = LoadTexture(CurrentWorld.SKY_TEXTURE_PATH);    // Load map texture
	sky_model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = sky_texture;
	SetTextureWrap(sky_texture, TEXTURE_WRAP_REPEAT); // important for tiling
	sky_model.transform = MatrixScale(1, 1, -1);
	vw->sky_model = sky_model;

	// END OF BUILDING THE VOXEL WORLD world (appearance and "physical" structure)

	
	printf(" \n\n R        E        L        O        A        D        E        D \n\n" );

}
