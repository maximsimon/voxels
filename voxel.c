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
	Camera camera = { 0 };
	camera.position = (Vector3){ -10.0f, 40.0f, -10.0f };    // Camera position
	camera.target = (Vector3){ -10.0f, 0.0f, 10.0f };    // Camera looking at point
	camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };          // Camera up vector (rotation towards target)
	camera.fovy = 45.0f;                                // Camera field-of-view Y
	camera.projection = CAMERA_PERSPECTIVE;             // Camera projection type

	// setup player
	Camera player = { 0 };
	player.position = (Vector3){ 0.0f, 0.0f, 0.0f };    // Camera position
	player.target = (Vector3){ 0.0f, 1.0f, 1.0f };    // Camera looking at point
	player.up = (Vector3){ 0.0f, 1.0f, 0.0f };          // Camera up vector (rotation towards target)
	player.fovy = 45.0f;                                // Camera field-of-view Y
	player.projection = CAMERA_PERSPECTIVE;             // Camera projection type
	
	// setup mesh
	int voxel_count = 0;
	
	Mesh mesh = { 0 };
	mesh.vertices = (float *)RL_MALLOC(0); 
	mesh.normals = (float *)RL_MALLOC(0);
	mesh.indices = (unsigned short *)RL_MALLOC(0);
	
	// get map of voxel world (1 - voxel, 0 - no voxel)
	chunkMap map = FetchChunkMap();				// chunk is 16 x 16 voxels

	// build world based on map
	for (int i = 0; i < 16; i++) {
		for (int j = 0; j < 16; j++) {
			if (map.map[j + i*16] == 1) {
				MeshVoxel(&mesh, (float)j, 0.0f, (float)i, &voxel_count);
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
	MeshVoxel(&player_mesh, 0.0f, 0.5f, 0.0f, &players_count);
	UploadMesh(&player_mesh, false);
	Model player_model = LoadModelFromMesh(player_mesh);                  // Load model from generated mesh
	
	char position_info[70];
	char mode_info[70];

	// variable for plaer or edit mode (1st person camera moves or 3rd person camera moves)
	bool player_mode = false;	

	while(!WindowShouldClose() && !IsKeyPressed(KEY_Q)) {
		
		if (IsKeyPressed(KEY_P)) {
			player_mode = !player_mode;
		}
		if (player_mode) CheckMovement1person(&player, map);
		else CheckMovementEdit(&camera, &player, map);
		
		BeginDrawing();	
			
			ClearBackground(RAYWHITE);

			BeginMode3D(camera);
		
				DrawModel(model, mazePosition, 1.0f, BLACK);
				DrawModel(player_model, player.position, 1.0f, RED);
				DrawGrid(1000, 1.0f);

			EndMode3D();
		        
			Vector2 screenPos = GetWorldToScreen(camera.target, camera);
			
			// draw title, current position, fps
			DrawText("VOXELS", 10, 10, 30, BLACK);
			
			sprintf(position_info, "Current Player Position: x=%.2f, y=%.2f, z=%.2f", player.position.x, player.position.y, player.position.z);
        		DrawText(position_info, 10, 50, 20, GRAY);
			
			sprintf(mode_info, "Player Mode? %d", player_mode);
			DrawText(mode_info, 10, 90, 20, GREEN);
			
			DrawFPS(10, 130);

		EndDrawing();

	}

	// clean up
	UnloadModel(model);
	CloseWindow();

	return 0;
}
