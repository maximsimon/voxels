// THIS IS MAIN FILE CONTROLLING THE VOXELS SIMULATION

#include "raylib.h"
#include "rlgl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "player_movement.hpp"
#include "god_movement.hpp"
#include "map.hpp"
#include "faces.hpp"
#include "master_voxel.hpp"
#include "support_for_master.hpp"
#include "small_handy_stuff.hpp"
#include "data_types.hpp"
#include "odometry.hpp"
#include "lidar.hpp"
#include "config_core.hpp"
#include "world_config.hpp"

// Draw an arrow at the agent's position pointing along its look direction
// IMPORTANT: never call this from inside the player_camera's
static void drawPlayer(Camera3D camera, Color color) {
	Vector3 forward = Vector3Subtract(camera.target, camera.position);
	forward.y = 0.0f;
	float len = sqrtf(forward.x * forward.x + forward.z * forward.z);
	if (len < 1e-3f) return;
	forward.x /= len;
	forward.z /= len;

	const float shaft_len = 0.7f;
	const float head_len  = 0.3f;
	const float shaft_r   = 0.15f;
	const float head_r    = 0.30f;

	Vector3 base = camera.position + forward * 0.5f;
	Vector3 shaft_end = (Vector3){
		base.x + forward.x * shaft_len,
		base.y,
		base.z + forward.z * shaft_len,
	};
	Vector3 tip = (Vector3){
		base.x + forward.x * (shaft_len + head_len),
		base.y,
		base.z + forward.z * (shaft_len + head_len),
	};
	Model cube = LoadModelFromMesh(GenMeshCube(1,1,1));

	// draw player cube
	DrawModelEx(
	    cube,
	    camera.position,
	    (Vector3){0,1,0},   // axis
	    getPlayerAngleDeg(camera),
	    (Vector3){1,1,1},
	    RED
	);

	// draw arrow to visualise player heading
	DrawCylinderEx(base, shaft_end, shaft_r, shaft_r, 8, color);
	DrawCylinderEx(shaft_end, tip,  head_r,  0.0f,    12, color);
}

// initilize simulation - allocate memory, create structs, define window size, etc
VoxelWorld *init_sim(Vector3 player_pose, Vector3 player_direction, int step_size) {
	
	//VoxelWorld *vw = (VoxelWorld*)malloc(sizeof(VoxelWorld));	
	VoxelWorld *vw = new VoxelWorld();
	// start window
	InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "You're in voxels now.");
	SetTargetFPS(100 / step_size);
	
	// setup camera
	Camera edit_camera = { 0 };
	edit_camera.position = (Vector3){ -10.0f, 40.0f, -10.0f };    // Camera position
	edit_camera.target = (Vector3){ -10.0f, 0.0f, 10.0f };    // Camera looking at point
	edit_camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };          // Camera up vector (rotation towards target)
	edit_camera.fovy = 45.0f;                                // Camera field-of-view Y
	edit_camera.projection = CAMERA_PERSPECTIVE;             // Camera projection type
	vw->edit_camera = edit_camera;
	
	// setup player
	Camera player_camera = { 0 };
	player_camera.position = player_pose;    // Camera position
	player_camera.target = {player_pose.x + player_direction.x, 0.5f, player_pose.z + player_direction.z};    // Camera looking at point
	player_camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };          // Camera up vector (rotation towards target)
	player_camera.fovy = 45.0f;                                // Camera field-of-view Y
	player_camera.projection = CAMERA_PERSPECTIVE;             // Camera projection type
	vw->player_camera = player_camera;

	// load which world to use from config file before building the world
	load_world_config("core_voxels/resources/worlds/worlds.config");
	vw->current_world = CurrentWorld;

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

	// mesh and model of the player
	Mesh player_mesh = { 0 };
	player_mesh.vertices = (float *)RL_MALLOC(0); 
	player_mesh.normals = (float *)RL_MALLOC(0);
	player_mesh.indices = (unsigned short *)RL_MALLOC(0);

	// mode variables
	bool player_mode = false;	
	bool player_view = false;
	Camera current_camera = edit_camera;
	vw->player_mode = player_mode;
	vw->player_view = player_view;
	vw->current_camera = current_camera;	
	
	// initilize rest of VoxelWorld vw	
	vw->camera_view_tex = LoadRenderTexture(SCREEN_WIDTH, SCREEN_HEIGHT);
	vw->teleport_text.text_active = false;
	vw->teleport_text.letter_count = 0;
	vw->teleport_text.text_box = { SCREEN_WIDTH - 400, 100, 200, 50 };		// TODO: magic numbers
	vw->teleport_text.MAX_INPUT_CHARS = 30;
	vw->teleport_text.text[vw->teleport_text.MAX_INPUT_CHARS] = { 0 };      // NOTE: One extra space required for null terminator char '\0'	
	printf("init succesfull\n");
	return vw;
}

// this function is one step of simulation, here happen all calculations of what happens in the simulation as well as the actual visual rendering, called in loop from master_main
void step_sim(VoxelWorld *vw, Action *action, Observation *observation) {
	// TODO: move these somewhere a bit cleaner
	char position_info[70];
	char mode_info[70];
    	Vector3 mazePosition = { 0.0f, 0.5f, 0.0f };           // Define model position

	// handle all keys pressed
	handleActionsAndKeys(vw, action, observation);	// movement based on keys stop movement on action until keys are released
	updateOdometry(vw, action, observation);	// currently twist msg inside odometry in observation is directly taken from action 
	updateLidar(vw, observation);		// cast LiDAR rays through the voxel grid	
	
	// screenshot
	char img_fname[64];
	sprintf(img_fname, "screenshot.png");
	if (IsKeyPressed(KEY_SPACE)) {
		TakeScreenshot(img_fname); 	
	}
	
	BeginTextureMode(vw->camera_view_tex);
		ClearBackground(RAYWHITE);
		BeginMode3D(vw->player_camera);
			DrawModel(vw->sky_model, (Vector3){0, 0, 0}, 1.0f, WHITE);		// draw the sky
			DrawModel(vw->ground_model, (Vector3){0, 0, 0}, 1.0f, WHITE);		// draw the ground

			// TODO get chunk_position of the ones surroundin the player and draw only them
			for (int i = 0; i < vw->main_map.width_chunks * vw->main_map.height_chunks; i++) {
				DrawModel(vw->maze_model[i], mazePosition, 1.0f, WHITE);
			}
		EndMode3D();
    	EndTextureMode();
	
	Image pov_view_img = LoadImageFromTexture(vw->camera_view_tex.texture);	
	ImageFlipVertical(&pov_view_img); 	
	
	BeginDrawing();	
		
		ClearBackground(RAYWHITE);
		
		BeginMode3D(vw->current_camera);
		
			DrawModel(vw->sky_model, (Vector3){0, 0, 0}, 1.0f, WHITE);		// draw the sky
			DrawModel(vw->ground_model, (Vector3){0, 0, 0}, 1.0f, WHITE);		// draw the ground
			// Rays first, then agent, so the player marker stays readable over scan lines.
			if (!(vw->player_mode && vw->player_view)) drawLidarRays(vw, observation, false);
			if (!(vw->player_mode && vw->player_view)) {
				drawPlayer(vw->player_camera, RED);				// draw heading arrow (the player)
			}
			
			// draw maze
			for (int i = 0; i < vw->main_map.width_chunks * vw->main_map.height_chunks; i++) {	// draw voxels
				DrawModel(vw->maze_model[i], mazePosition, 1.0f, WHITE);
			}
			
			// for debugging: DrawGrid(1000, 1.0f);
			
			// TODO: draw cross in the middle of the screen (put toggle on/off in config)
			// TODO: fix allocating variables in loop

		EndMode3D();
		
		Vector2 screenPos = GetWorldToScreen(vw->current_camera.target, vw->current_camera);
		
		// draw title, current position, fps
		DrawText("VOXELS", 10, 10, 30, BLACK);
		
		sprintf(position_info, "Current Player Position: x=%.2f, y=%.2f, z=%.2f", vw->player_camera.position.x, vw->player_camera.position.y, vw->player_camera.position.z);
		DrawText(position_info, 10, 50, 20, RED);
		
		sprintf(mode_info, "Player Mode? %d Player View? %d", vw->player_mode, vw->player_view);
		DrawText(mode_info, 10, 90, 20, BLACK);
		DrawFPS(10, 130);

		// draw teleport input box if T was pressed
		if (vw->teleport_text.text_active == true) {
			DrawRectangleRec(vw->teleport_text.text_box, (Color){0, 0, 0, 0});
			DrawRectangleLines((int)vw->teleport_text.text_box.x, (int)vw->teleport_text.text_box.y, (int)vw->teleport_text.text_box.width, (int)vw->teleport_text.text_box.height, DARKGRAY);
			DrawText(vw->teleport_text.text, (int)vw->teleport_text.text_box.x + 5, (int)vw->teleport_text.text_box.y + 8, 25, GREEN);
			DrawText(TextFormat("teleport: x z yaw_deg"), (int)vw->teleport_text.text_box.x + 5, vw->teleport_text.text_box.y + 40, 13, GREEN);
		}
	
		//TODO: you can draw camera front POV in a little window at bottom right like this (only need to scale down the texture):	
		//DrawTextureRec(
		//	vw->camera_view_tex.texture,
		//	(Rectangle){ 0, 0, vw->camera_view_tex.texture.width, -vw->camera_view_tex.texture.height },
		//	(Vector2){ 500, 400 },
		//	WHITE
		//);
		

	EndDrawing();
	
	// Convert current robot POV view (front camera) from type raylib Image to cv2::Mat
	cv::Mat mat_temp(pov_view_img.height, pov_view_img.width, CV_8UC4, pov_view_img.data); // RGBA
	cv::Mat pov_view_cvimg;
	mat_temp.copyTo(pov_view_cvimg);   // <-- deep copy - so that i can Unload image
	cv::cvtColor(pov_view_cvimg, pov_view_cvimg, cv::COLOR_RGBA2BGR);	
	observation->camera_front = pov_view_cvimg;

	UnloadImage(pov_view_img);		// TODO: if i unload the image, the cv points to empty thing, check if not unloading the image doesnt cause some ugly leaks that slow down stuff or something
	
}

// TODO - cleanup
void end_sim(void) {

	// clean up		//todo: should free(main_map)
	//UnloadModel(model);
	//rlBindFramebuffer(0); // bind default framebuffer
	//free(color_view_pixels) or something
	CloseWindow();
}

