// VOXELS: PROGRAMMING MY RENDERING OF VOXEL WORLD, starting with a MAZE FROM PICTURE AND MY KEYBINDINGS FOR MOVEMENT etc

#include "raylib.h"
#include <stdio.h>
#include "camera_movement.h"
//#include "meshes.h"
#include "map.h"
#include "single_voxels.h"

const int screen_width = 1600;
const int screen_height = 850;


//Mesh MainErection(Mesh *maze_mesh, chunkMap maze_map, int wall);		// builds maze from 3D array map, where 1 = wall of <wall> voxels, 0 is nothing
Mesh MainErection(chunkMap maze_map, int wall);		// changed for OGMeshCube - temp 

int main(void){

	InitWindow(screen_width, screen_height, "You're in voxels now.");
	SetTargetFPS(60);
	
	Camera camera = { 0 };
	camera.position = (Vector3){ 1.0f, 1.0f, 1.0f };    // Camera position
	camera.target = (Vector3){ 0.0f, 1.0f, 0.0f };    // Camera looking at point
	camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };          // Camera up vector (rotation towards target)
	camera.fovy = 45.0f;                                // Camera field-of-view Y
	camera.projection = CAMERA_PERSPECTIVE;             // Camera projection type

	chunkMap map = FetchChunkMap();				// fetch map of voxels in 16 x 16 chunk
	
	Mesh mesh = MainErection(map, 1);

	//for (int i = 0; i < 24; i++) {
	//	printf("%f ", mesh.vertices[i]);
	//}
	//printf("\n");
	//for (int i = 0; i < 24; i++) {
	//	printf("%f ", mesh.normals[i]);
	//}
	//printf("\n");
	//for (int i = 0; i < 36; i++) {
	//	printf("%d ", mesh.indices[i]);
	//}
	//printf("\n");
	


	UploadMesh(&mesh, false);
	
	Model model = LoadModelFromMesh(mesh);                  // Load model from generated mesh
    	Vector3 mazePosition = { 0.0f, 0.5f, 0.0f };           // Define model position

	while(!WindowShouldClose() && !IsKeyPressed(KEY_Q)) {
		
		CheckMovement(&camera);

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

Mesh MainErection(chunkMap maze_map, int wall) {
	//maze_mesh->vertexCount = 0;
	//maze_mesh->triangleCount = 0;

	//maze_mesh.vertices = (float *)RL_MALLOC(8 * 3 * sizeof(int));
    	//maze_mesh.normals = (float *)RL_MALLOC(8 * 3 * sizeof(float));
    
	int chunk_voxel_num = 0; 
	for (int i = 0; i < chunk_w * chunk_h; i++) if (maze_map.map[i] == 1) chunk_voxel_num ++;

	/*
	maze_mesh->vertices = (float *)calloc(24*3, sizeof(float));
	maze_mesh->normals = (float *)calloc(24*3, sizeof(float));
	maze_mesh->indices = (unsigned short *)calloc(36*3, sizeof(unsigned short));
	*/
		
	// *maze_mesh = MeshVoxel(maze_mesh, -2.0f, 0.5f, -2.0f);
   
	Mesh maze_mesh = OGMeshCube(1.0f, 0.0f, 1.0f);

	//for (int z = 0; z < chunk_h; z++) {
	//	for (int x = 0; x < chunk_w; x++) {
	//		if (maze_map.map[x + z * chunk_h] == 1) {
	//			for (int y = 0; y < wall; y++) maze_mesh = MeshVoxel(&maze_mesh, (float)x, (float)y, (float)z);
	//		}
	//	}
	//}
	//mesh = MeshVoxel(&mesh, 0.0f, 0.5f, 0.0f);

	return maze_mesh;
}

