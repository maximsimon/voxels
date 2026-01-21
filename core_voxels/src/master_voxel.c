// THIS IS MAIN
// VOXELS: PROGRAMMING MY RENDERING OF VOXEL WORLD, starting with a MAZE FROM PICTURE AND MY KEYBINDINGS FOR MOVEMENT, then connecting to ROS and more

#include "raylib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "player_movement.h"
#include "camera_movement.h"
#include "map.h"
#include "faces.h"
#include "master_voxel.h"

struct VoxelWorld {
	bool player_view;		// controls if player or edit POV is viewed
	bool player_mode;		// controls if arrow keys control player voxel or flying edit camera
	Camera3D edit_camera;		// flying "god mode" camera
	Camera3D player_camera;		// player (robot) camera
	Camera3D current_camera;	// player or edit camera - it is the one that is currently being rendered
	mainMap main_map;		// array map of the world
	Model maze_model;		// model of the world
	Mesh maze_mesh;			// mesh of the world;
	Model player_model;		// 1 red voxel - model of the player (robot)
	float player_angle;
};

struct Observation{
	int odom;
};

struct Action{
	Vector3 movement_direction;
};

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
	
	return vw;
}

Observation *step_sim(VoxelWorld *vw, Action *action) {
	// TODO: move these somewhere a bit cleaner
	char position_info[70];
	char mode_info[70];
	int curr_chunk = fetchCurrChunkId(vw->main_map);
	Vector3 y_axis = {0.0f, 1.0f, 0.0f};
	Vector3 scale = {1.0f, 1.0f, 1.0f};
    	Vector3 mazePosition = { 0.0f, 0.5f, 0.0f };           // Define model position
	
	if (IsKeyPressed(KEY_P)) {
		vw->player_mode = !vw->player_mode;
	}
	if (IsKeyPressed(KEY_V)) {
		vw->player_view = !vw->player_view;
		vw->player_angle = getPlayerAngle(vw->player_camera);
	}

	// control player_camera and Movement1person
	if (vw->player_mode && vw->player_view) {
		CheckMovement1person(&vw->player_camera, vw->main_map.chunks[curr_chunk], &vw->maze_mesh);
		vw->current_camera = vw->player_camera;
		vw->player_angle = getPlayerAngle(vw->player_camera);
		
	// control edit_camera but Movement1person
	} else if(vw->player_mode) {
		CheckMovement1person(&vw->player_camera, vw->main_map.chunks[curr_chunk], &vw->maze_mesh);
		vw->current_camera = vw->edit_camera;
		vw->player_angle = getPlayerAngle(vw->player_camera);
	// control edit_camera and MovementEdit
	} else {
		CheckMovementEdit(&vw->edit_camera, vw->main_map.chunks[curr_chunk]);
		vw->current_camera = vw->edit_camera;
	}
	
	BeginDrawing();	
		
		ClearBackground(RAYWHITE);
		
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
		
		// changing variables

	EndDrawing();

}

void end_sim(void) {

	// clean up		//todo: should free(main_map)
	//UnloadModel(model);
	CloseWindow();
}

