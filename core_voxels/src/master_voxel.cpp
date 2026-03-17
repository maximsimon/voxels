// THIS IS MAIN
// VOXELS: PROGRAMMING MY RENDERING OF VOXEL WORLD, starting with a MAZE FROM PICTURE AND MY KEYBINDINGS FOR MOVEMENT, then connecting to ROS and more

#include "raylib.h"
#include "rlgl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "player_movement.hpp"
#include "camera_movement.hpp"
#include "map.hpp"
#include "faces.hpp"
#include "master_voxel.hpp"
#include "support_for_master.hpp"
#include "data_types.hpp"
#include "odometry.hpp"

//window size
const int screen_width = 1600;
const int screen_height = 850;

VoxelWorld *init_sim(Image mazemap_image, Vector3 player_pose, Vector3 player_direction, int step_size) {
	
	VoxelWorld *vw = (VoxelWorld*)malloc(sizeof(VoxelWorld));	

	// start window
	InitWindow(screen_width, screen_height, "You're in voxels now.");
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
	player_camera.position = (Vector3){ -2.0f, 0.5f, -2.0f };    // Camera position
	player_camera.target = (Vector3){ -2.0f, 0.5f, -1.0f };    // Camera looking at point
	player_camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };          // Camera up vector (rotation towards target)
	player_camera.fovy = 45.0f;                                // Camera field-of-view Y
	player_camera.projection = CAMERA_PERSPECTIVE;             // Camera projection type
	vw->player_camera = player_camera;
	// setup mesh
	int voxel_count = 0;
	
	Mesh mesh = { 0 };
	mesh.vertices = (float *)RL_MALLOC(0); 
	mesh.normals = (float *)RL_MALLOC(0);
	mesh.indices = (unsigned short *)RL_MALLOC(0);
	
	// get map of voxel world (1 - voxel, 0 - no voxel)
	mainMap main_map = fetchMainMap(mazemap_image);
	vw->main_map = main_map;
	//MeshVoxel(&mesh, 1.0f, 0.0f, 1.0f, 0.0f, &voxel_count);
	// build world based on map
	for (int ch_z = 0; ch_z < main_map.height_chunks; ch_z++) {
		for (int ch_x = 0; ch_x < main_map.width_chunks; ch_x++) {
			for (int z = 0; z < main_map.chunk_side; z++) {
				for (int x = 0; x < main_map.chunk_side; x++) {
					if (main_map.chunks[ch_z * main_map.width_chunks + ch_x].map[z * main_map.chunk_side + x] == 1) {
						MeshVoxel(&mesh, (float)(ch_x * main_map.chunk_side + x), 0.0f, (float)(ch_z * main_map.chunk_side + z), 0.0f, &voxel_count);
					}
				}
			}
		}
	}

	vw->maze_mesh = mesh;
	// upload world
	UploadMesh(&mesh, false);
	
	// model of the world
	Model model = LoadModelFromMesh(mesh);                  // Load model from generated mesh
	vw->maze_model = model;

	// mesh and model of the player
	Mesh player_mesh = { 0 };
	player_mesh.vertices = (float *)RL_MALLOC(0); 
	player_mesh.normals = (float *)RL_MALLOC(0);
	player_mesh.indices = (unsigned short *)RL_MALLOC(0);
	
	//TODO: from use player pose and player direction parameters
	int players_count = 0;
	float player_angle = 0.5f;
	Vector3 y_axis = {0.0f, 1.0f, 0.0f};
	Vector3 scale = {1.0f, 1.0f, 1.0f};

	MeshVoxel(&player_mesh, 0.0f, 0.5f, 0.0f, player_angle, &players_count);
	UploadMesh(&player_mesh, false);
	Model player_model = LoadModelFromMesh(player_mesh);                  // Load model from generated mesh
	vw->player_model = player_model;	

	// mode variables
	bool player_mode = false;	
	bool player_view = false;
	Camera current_camera = edit_camera;
	vw->player_mode = player_mode;
	vw->player_view = player_view;
	vw->current_camera = current_camera;	
	
	vw->camera_view_tex = LoadRenderTexture(screen_width, screen_height);
	return vw;
}

//Observation *step_sim(VoxelWorld *vw, Action *action) {
void step_sim(VoxelWorld *vw, Action *action, Observation *observation) {
	// TODO: move these somewhere a bit cleaner
	char position_info[70];
	char mode_info[70];
	int curr_chunk = fetchCurrChunkId(vw->main_map);
	Vector3 y_axis = {0.0f, 1.0f, 0.0f};
	Vector3 scale = {1.0f, 1.0f, 1.0f};
    	Vector3 mazePosition = { 0.0f, 0.5f, 0.0f };           // Define model position

	// handle all keys pressed
	handleActionsAndKeys(vw, curr_chunk, action);	// movement based on keys stop movement on action until keys are released
	updateOdometry(vw, action, observation);	// currently twist msg inside odometry in observation is directly taken from action -> TODO: actually caluclate player movemetn in the simulaiton	
	BeginTextureMode(vw->camera_view_tex);
		ClearBackground(RAYWHITE);
		BeginMode3D(vw->player_camera);
			DrawModelEx(vw->player_model, vw->player_camera.position, y_axis, vw->player_angle, scale, RED);
			DrawModel(vw->maze_model, mazePosition, 1.0f, BLACK);
			DrawGrid(1000, 1.0f);
		EndMode3D();
    	EndTextureMode();
	

	BeginDrawing();	
		
		ClearBackground(RAYWHITE);
		Image pov_view_img = LoadImageFromTexture(vw->camera_view_tex.texture);	
		ImageFlipVertical(&pov_view_img); 	
		
		BeginMode3D(vw->current_camera);
		
			DrawModelEx(vw->player_model, vw->player_camera.position, y_axis, vw->player_angle, scale, RED);
	
			DrawModel(vw->maze_model, mazePosition, 1.0f, BLACK);
			DrawGrid(1000, 1.0f);

		EndMode3D();
		
		Vector2 screenPos = GetWorldToScreen(vw->current_camera.target, vw->current_camera);
		
		// draw title, current position, fps
		DrawText("VOXELS", 10, 10, 30, BLACK);
		
		sprintf(position_info, "Current Player Position: x=%.2f, y=%.2f, z=%.2f", vw->player_camera.position.x, vw->player_camera.position.y, vw->player_camera.position.z);
		DrawText(position_info, 10, 50, 20, GRAY);
		
		sprintf(mode_info, "Player Mode? %d Player View? %d", vw->player_mode, vw->player_view);
		DrawText(mode_info, 10, 90, 20, GREEN);
		
		DrawFPS(10, 130);
		
		//TODO: you can draw camera front POV in a little window at bottom right like this (only need to scale down the texture):	
		/*DrawTextureRec(
			vw->camera_view_tex.texture,
			(Rectangle){ 0, 0, vw->camera_view_tex.texture.width, -vw->camera_view_tex.texture.height },
			(Vector2){ 500, 400 },
			WHITE
		);*/
		
		// changing variables

	EndDrawing();

        // Capture the whole screen after drawing
        //Image screenshot = LoadImageFromScreen();
	
	// to easily view camera_view_tex for debugging
	//Image img = LoadImageFromTexture(camera_view_tex.texture);	
	//ExportImage(img, "img.png");

	// Convert current robot POV view (front camera) from type raylib Image to cv2::Mat
	
	cv::Mat mat_temp(pov_view_img.height, pov_view_img.width, CV_8UC4, pov_view_img.data); // RGBA
	cv::Mat pov_view_cvimg;
	mat_temp.copyTo(pov_view_cvimg);   // <-- deep copy - so that i can Unload image
	cv::cvtColor(pov_view_cvimg, pov_view_cvimg, cv::COLOR_RGBA2BGR);	
	observation->camera_front = pov_view_cvimg;

	UnloadImage(pov_view_img);		// TODO: if i unload the image, the cv points to empty thing, check if not unloading the image doesnt cause some ugly leaks that slow down stuff or something
	
	//Observation *obs = new Observation();
	//return obs;
}

void end_sim(void) {

	// clean up		//todo: should free(main_map)
	//UnloadModel(model);
	//rlBindFramebuffer(0); // bind default framebuffer
	//free(color_view_pixels) or something
	CloseWindow();
}

