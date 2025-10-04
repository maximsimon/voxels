// THIS IS MAIN
// VOXELS: PROGRAMMING MY RENDERING OF VOXEL WORLD, starting with a MAZE FROM PICTURE AND MY KEYBINDINGS FOR MOVEMENT, then connecting to ROS and more

#include "raylib.h"
#include <stdio.h>
#include "player_movement.h"
#include "camera_movement.h"
#include "map.h"
#include <stdlib.h>
#include <string.h>
#include "faces.h"

// window size
const int screen_width = 1600;
const int screen_height = 850;

int main(void){

	// start window
	InitWindow(screen_width, screen_height, "You're in voxels now.");
	SetTargetFPS(60);
	
	// setup camera
	Camera edit_camera = { 0 };
	edit_camera.position = (Vector3){ -10.0f, 40.0f, -10.0f };    // Camera position
	edit_camera.target = (Vector3){ -10.0f, 0.0f, 10.0f };    // Camera looking at point
	edit_camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };          // Camera up vector (rotation towards target)
	edit_camera.fovy = 45.0f;                                // Camera field-of-view Y
	edit_camera.projection = CAMERA_PERSPECTIVE;             // Camera projection type

	// setup player
	Camera player_camera = { 0 };
	player_camera.position = (Vector3){ -2.0f, 0.5f, -2.0f };    // Camera position
	player_camera.target = (Vector3){ -2.0f, 0.5f, -1.0f };    // Camera looking at point
	player_camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };          // Camera up vector (rotation towards target)
	player_camera.fovy = 45.0f;                                // Camera field-of-view Y
	player_camera.projection = CAMERA_PERSPECTIVE;             // Camera projection type
	
	// setup mesh
	int voxel_count = 0;
	
	Mesh mesh = { 0 };
	mesh.vertices = (float *)RL_MALLOC(0); 
	mesh.normals = (float *)RL_MALLOC(0);
	mesh.indices = (unsigned short *)RL_MALLOC(0);
	
	// get map of voxel world (1 - voxel, 0 - no voxel)
	chunkMap map = FetchChunkMap();				// chunk is 16 x 16 voxels

	//MeshVoxel(&mesh, 1.0f, 0.0f, 1.0f, 0.0f, &voxel_count);
	// build world based on map
	for (int i = 0; i < 16; i++) {
		for (int j = 0; j < 16; j++) {
			if (map.map[j + i*16] == 1) {
				MeshVoxel(&mesh, (float)j, 0.0f, (float)i, 0.0f, &voxel_count);
			}
		}
	}

	// upload world
	UploadMesh(&mesh, false);
	
	// model of the world
	Model model = LoadModelFromMesh(mesh);                  // Load model from generated mesh
    	Vector3 mazePosition = { 0.0f, 0.5f, 0.0f };           // Define model position

	// mesh and model of the player
	Mesh player_mesh = { 0 };
	player_mesh.vertices = (float *)RL_MALLOC(0); 
	player_mesh.normals = (float *)RL_MALLOC(0);
	player_mesh.indices = (unsigned short *)RL_MALLOC(0);
	
	// build world based on map
	int players_count = 0;
	float player_angle = 0.5f;
	Vector3 y_axis = {0.0f, 1.0f, 0.0f};
	Vector3 scale = {1.0f, 1.0f, 1.0f};

	MeshVoxel(&player_mesh, 0.0f, 0.5f, 0.0f, player_angle, &players_count);
	UploadMesh(&player_mesh, false);
	Model player_model = LoadModelFromMesh(player_mesh);                  // Load model from generated mesh
	
	char position_info[70];
	char mode_info[70];

	// mode variables
	bool player_mode = false;	
	bool player_view = false;
	Camera current_camera = edit_camera;

	while(!WindowShouldClose() && !IsKeyPressed(KEY_Q)) {
		
		if (IsKeyPressed(KEY_P)) {
			player_mode = !player_mode;
		}
		if (IsKeyPressed(KEY_V)) {
			player_view = !player_view;
			player_angle = getPlayerAngle(player_camera);
		}

		if (player_mode && player_view) {
			CheckMovement1person(&player_camera, map, &mesh);
			current_camera = player_camera;
			player_angle = getPlayerAngle(player_camera);
			
		} else if(player_mode) {
			CheckMovement1person(&player_camera, map, &mesh);
			current_camera = edit_camera;
			player_angle = getPlayerAngle(player_camera);
		} else {
			CheckMovementEdit(&edit_camera, map);
			current_camera = edit_camera;
		}
		
		BeginDrawing();	
			
			ClearBackground(RAYWHITE);
			
			BeginMode3D(current_camera);
			
				DrawModelEx(player_model, player_camera.position, y_axis, player_angle, scale, RED);
		
				DrawModel(model, mazePosition, 1.0f, BLACK);
				DrawGrid(1000, 1.0f);

			EndMode3D();
		        
			Vector2 screenPos = GetWorldToScreen(current_camera.target, current_camera);
			
			// draw title, current position, fps
			DrawText("VOXELS", 10, 10, 30, BLACK);
			
			sprintf(position_info, "Current Player Position: x=%.2f, y=%.2f, z=%.2f", player_camera.position.x, player_camera.position.y, player_camera.position.z);
        		DrawText(position_info, 10, 50, 20, GRAY);
			
			sprintf(mode_info, "Player Mode? %d Player View? %d", player_mode, player_view);
			DrawText(mode_info, 10, 90, 20, GREEN);
			
			DrawFPS(10, 130);
			
			// changing variables

		EndDrawing();

	}

	// clean up
	UnloadModel(model);
	CloseWindow();

	return 0;
}
