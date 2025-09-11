// THIS IS MAIN
// VOXELS: PROGRAMMING MY RENDERING OF VOXEL WORLD, starting with a MAZE FROM PICTURE AND MY KEYBINDINGS FOR MOVEMENT, then connecting to ROS and more

#include "raylib.h"
#include <stdio.h>
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
	camera.position = (Vector3){ 1.0f, 1.0f, 1.0f };    // Camera position
	camera.target = (Vector3){ 0.0f, 1.0f, 1.0f };    // Camera looking at point
	camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };          // Camera up vector (rotation towards target)
	camera.fovy = 45.0f;                                // Camera field-of-view Y
	camera.projection = CAMERA_PERSPECTIVE;             // Camera projection type

	// setup mesh
	int voxel_count = 0;
	
	Mesh mesh = { 0 };
	mesh.vertices = (float *)RL_MALLOC(0); 
	mesh.normals = (float *)RL_MALLOC(0);
	mesh.indices = (unsigned short *)RL_MALLOC(0);
	
	// get map of voxel world (1 - voxel, 0 - no voxel)
	chunkMap map = FetchChunkMap();				// chunk is 16 x 16 voxels

	// build world base on map
	for (int i = 0; i < 16; i++) {
		for (int j = 0; j < 16; j++) {
			if (map.map[j + i*16] == 1) {
				MeshVoxel(&mesh, (float)i, 0.0f, (float)j, &voxel_count);
			}
		}
	}

	// upload world
	UploadMesh(&mesh, false);
	
	Model model = LoadModelFromMesh(mesh);                  // Load model from generated mesh
    	Vector3 mazePosition = { 0.0f, 0.5f, 0.0f };           // Define model position
	

	while(!WindowShouldClose() && !IsKeyPressed(KEY_Q)) {
		
		CheckMovement(&camera, map);

		BeginDrawing();	
			
			ClearBackground(RAYWHITE);

			BeginMode3D(camera);
		
				DrawModel(model, mazePosition, 1.0f, BLACK);
				DrawGrid(1000, 1.0f);

			EndMode3D();
		        
			Vector2 screenPos = GetWorldToScreen(camera.target, camera);
			DrawText("VOXELS", 10, 10, 30, GRAY);
			DrawFPS(10, 50);

		EndDrawing();

	}

	// clean up
	UnloadModel(model);
	CloseWindow();

	return 0;
}
