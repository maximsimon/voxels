// THIS IS MAIN
// VOXELS: PROGRAMMING MY RENDERING OF VOXEL WORLD, starting with a MAZE FROM PICTURE AND MY KEYBINDINGS FOR MOVEMENT, then connecting to ROS and more

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

//window size
const int screen_width = 1600;
const int screen_height = 850;

// Draw an arrow at the agent's position pointing along its look direction
// (camera.target - camera.position projected to the floor plane).  Used in
// the on-screen god/edit view to make the agent's heading visible at a
// glance.
//
// IMPORTANT: never call this from inside the player_camera's
// `camera_view_tex` rendering pass.  That texture becomes the
// `camera_front_publisher` image consumed by the navigation NN.  The arrow
// shaft extends forward from the camera position, so it lands in the near
// plane and turns the navigation input into a red blob — control diverges
// instantly.  In a first-person POV there is no agent to render anyway.
static void drawAgentArrow(Camera3D camera, Color color) {
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

	Vector3 base = camera.position;
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

	DrawCylinderEx(base, shaft_end, shaft_r, shaft_r, 8, color);
	DrawCylinderEx(shaft_end, tip,  head_r,  0.0f,    12, color);
}

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
	player_camera.position = (Vector3){ 21.0f, 0.5f, 6.0f };    // Camera position
	player_camera.target = (Vector3){ -2.0f, 0.5f, -1.0f };    // Camera looking at point
	player_camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };          // Camera up vector (rotation towards target)
	player_camera.fovy = 45.0f;                                // Camera field-of-view Y
	player_camera.projection = CAMERA_PERSPECTIVE;             // Camera projection type
	vw->player_camera = player_camera;

	// build map and Voxel World	
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
	Texture2D texture = LoadTexture("src/ros_voxels/core_voxels/resources/textures/atlas.png");    // Load map texture
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
	Texture2D groundTex = LoadTexture("src/ros_voxels/core_voxels/resources/textures/grass.jpg");
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
	Texture2D sky_texture = LoadTexture("src/ros_voxels/core_voxels/resources/textures/stars.png");    // Load map texture
	sky_model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = sky_texture;
	SetTextureWrap(sky_texture, TEXTURE_WRAP_REPEAT); // important for tiling
	sky_model.transform = MatrixScale(1, 1, -1);
	vw->sky_model = sky_model;

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

	MeshVoxel(player_mesh, 0.0f, 0.5f, 0.0f, player_angle, &players_count, 0, 1.0f, 1.0f, 1.0f);
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
	
	// initilize rest of VoxelWorld vw	
	vw->camera_view_tex = LoadRenderTexture(screen_width, screen_height);
	vw->teleport_text.text_active = false;
	vw->teleport_text.letter_count = 0;
	vw->teleport_text.text_box = { screen_width - 400, 100, 200, 50 };
	vw->teleport_text.MAX_INPUT_CHARS = 30;		// TODO: magic number
	vw->teleport_text.text[vw->teleport_text.MAX_INPUT_CHARS] = { 0 };      // NOTE: One extra space required for null terminator char '\0'	
	printf("init succesfull\n");
	return vw;
}

//Observation *step_sim(VoxelWorld *vw, Action *action) {
void step_sim(VoxelWorld *vw, Action *action, Observation *observation) {
	// TODO: move these somewhere a bit cleaner
	char position_info[70];
	char mode_info[70];
	//int curr_chunk = fetchCurrChunkId(vw->main_map);
	Vector3 y_axis = {0.0f, 1.0f, 0.0f};
	Vector3 scale = {1.0f, 1.0f, 1.0f};
    	Vector3 mazePosition = { 0.0f, 0.5f, 0.0f };           // Define model position

	// handle all keys pressed
	handleActionsAndKeys(vw, action, observation);	// movement based on keys stop movement on action until keys are released
	updateOdometry(vw, action, observation);	// currently twist msg inside odometry in observation is directly taken from action -> TODO: actually caluclate player movemetn in the simulaiton	
	
	BeginTextureMode(vw->camera_view_tex);
		ClearBackground(RAYWHITE);
		BeginMode3D(vw->player_camera);
			DrawModel(vw->sky_model, (Vector3){0, 0, 0}, 1.0f, WHITE);		// draw the sky
			DrawModel(vw->ground_model, (Vector3){0, 0, 0}, 1.0f, WHITE);		// draw the ground
			// Intentionally do not draw the agent in its own first-person
			// POV — this texture is the navigation NN's camera input, and
			// any geometry placed at camera.position would obscure it.

			// TODO get chunk_position of the ones surroundin the player and draw only them
			for (int i = 0; i < vw->main_map.width_chunks * vw->main_map.height_chunks; i++) {
				DrawModel(vw->maze_model[i], mazePosition, 1.0f, WHITE);
			}
			//DrawGrid(1000, 1.0f);
		EndMode3D();
    	EndTextureMode();
	
	Image pov_view_img = LoadImageFromTexture(vw->camera_view_tex.texture);	
	ImageFlipVertical(&pov_view_img); 	
	
	BeginDrawing();	
		
		ClearBackground(RAYWHITE);
		
		BeginMode3D(vw->current_camera);
		
			DrawModel(vw->sky_model, (Vector3){0, 0, 0}, 1.0f, WHITE);		// draw the sky
			DrawModel(vw->ground_model, (Vector3){0, 0, 0}, 1.0f, WHITE);		// draw the ground
			// Heading arrow.  Skip in first-person on-screen view (the
			// camera is on top of the arrow, which would obscure the user's
			// own POV — same near-plane issue as the camera_front texture).
			if (!(vw->player_mode && vw->player_view)) {
				drawAgentArrow(vw->player_camera, RED);
			}
			
			// draw cross
			// TODO: fix allocating vrs in loop
			//int size = 20; // length of each arm
			//DrawLine((screen_width / 5) - size, (screen_height / 5), (screen_width / 5) + size, (screen_height / 5), BLACK); // horizontal
			//DrawLine((screen_width / 5), (screen_height / 5) - size, (screen_width / 5), (screen_height / 5) + size, BLACK); // vertica			for (int i = 0; i < vw->main_map.width_chunks * vw->main_map.height_chunks; i++) {	// draw voxels
			

			// draw maze
			for (int i = 0; i < vw->main_map.width_chunks * vw->main_map.height_chunks; i++) {	// draw voxels
				DrawModel(vw->maze_model[i], mazePosition, 1.0f, WHITE);
			}

		EndMode3D();
		
		Vector2 screenPos = GetWorldToScreen(vw->current_camera.target, vw->current_camera);
		
		// draw title, current position, fps
		DrawText("VOXELS", 10, 10, 30, BLACK);
		
		sprintf(position_info, "Current Player Position: x=%.2f, y=%.2f, z=%.2f", vw->player_camera.position.x, vw->player_camera.position.y, vw->player_camera.position.z);
		DrawText(position_info, 10, 50, 20, RED);
		
		sprintf(mode_info, "Player Mode? %d Player View? %d", vw->player_mode, vw->player_view);
		DrawText(mode_info, 10, 90, 20, GREEN);
		DrawFPS(10, 130);
		// draw teleport input box if T was pressed
		if (vw->teleport_text.text_active == true) {
			DrawRectangleRec(vw->teleport_text.text_box, (Color){0, 0, 0, 0});
			DrawRectangleLines((int)vw->teleport_text.text_box.x, (int)vw->teleport_text.text_box.y, (int)vw->teleport_text.text_box.width, (int)vw->teleport_text.text_box.height, DARKGRAY);
			DrawText(vw->teleport_text.text, (int)vw->teleport_text.text_box.x + 5, (int)vw->teleport_text.text_box.y + 8, 25, WHITE);
			DrawText(TextFormat("teleport: x z yaw_deg"), (int)vw->teleport_text.text_box.x + 5, vw->teleport_text.text_box.y + 40, 13, WHITE);
		}
		// Input format on T-key: "x z yaw_deg" — x/z are floor-plane meters
		// (raylib y is up, height pinned to 0.5); yaw_deg is the heading in
		// degrees (0 = +x, 90 = +z). If only "x z" is entered, the current
		// heading is preserved.
	
		//TODO: you can draw camera front POV in a little window at bottom right like this (only need to scale down the texture):	
		//DrawTextureRec(
		//	vw->camera_view_tex.texture,
		//	(Rectangle){ 0, 0, vw->camera_view_tex.texture.width, -vw->camera_view_tex.texture.height },
		//	(Vector2){ 500, 400 },
		//	WHITE
		//);
		
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

