// VOXELS: PROGRAMMING MY RENDERING OF VOXEL WORLD, starting with a MAZE FROM PICTURE AND MY KEYBINDINGS FOR MOVEMENT etc

#include "raylib.h"
#include <stdio.h>
#include "camera_movement.h"
//#include "meshes.h"
#include <stdlib.h>
#include <string.h>
#include "faces.h"

const int screen_width = 1600;
const int screen_height = 850;

Mesh MeshVoxel(Mesh *mesh, float width, float height, float length, float pos_x, float pos_y, float pos_z, int *voxel_count);	// generate one cube at the origin

int main(void){

	InitWindow(screen_width, screen_height, "You're in voxels now.");
	SetTargetFPS(60);
	
	Camera camera = { 0 };
	camera.position = (Vector3){ 1.0f, 1.0f, 1.0f };    // Camera position
	camera.target = (Vector3){ 0.0f, 1.0f, 1.0f };    // Camera looking at point
	camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };          // Camera up vector (rotation towards target)
	camera.fovy = 45.0f;                                // Camera field-of-view Y
	camera.projection = CAMERA_PERSPECTIVE;             // Camera projection type

	int voxel_count = 0;
	
	Mesh mesh = { 0 };
	mesh.vertices = (float *)RL_MALLOC(0); 
	mesh.normals = (float *)RL_MALLOC(0);
	mesh.indices = (unsigned short *)RL_MALLOC(0);

	MeshVoxel(&mesh, 1.0f, 1.0f, 1.0f, -5.0f, 2.0f, -4.0f, &voxel_count);
	printf("voxel count %d\n", voxel_count);
	MeshVoxel(&mesh, 1.0f, 1.0f, 1.0f, -3.0f, 1.0f, -4.0f, &voxel_count);
	printf("voxel count %d\n", voxel_count);
	
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

Mesh MeshVoxel(Mesh *mesh, float width, float height, float length, float pos_x, float pos_y, float pos_z, int *voxel_count) {

	// v dictenory (8 prvku) a n dictenory (6 prvnku)

	Vector3 v0 = { -width/2, -height/2,  length/2 };
	Vector3 v1 = {  width/2, -height/2,  length/2 };
	Vector3 v2 = {  width/2,  height/2,  length/2 };
	Vector3 v3 = { -width/2,  height/2,  length/2 };
	Vector3 v4 = { -width/2, -height/2, -length/2 };
	Vector3 v5 = { -width/2,  height/2, -length/2 };
	Vector3 v6 = {  width/2,  height/2, -length/2 };
	Vector3 v7 = {  width/2, -height/2, -length/2 };

	Vector3 n0 = { 1.0f, 0.0f, 0.0f };
    	Vector3 n1 = { -1.0f, 0.0f, 0.0f };
    	Vector3 n2 = { 0.0f, 1.0f, 0.0f };
    	Vector3 n3 = { 0.0f, -1.0f, 0.0f };
    	Vector3 n4 = { 0.0f, 0.0f, -1.0f };
    	Vector3 n5 = { 0.0f, 0.0f, 1.0f };

	Vector3 v_vectors[8] = { v0, v1, v2, v3, v4, v5, v6, v7 };
	Vector3 n_vectors[6] = { n0, n1, n2, n3, n4, n5 };

	int voxel_v_count = 0;
	int voxel_t_count = 0;

	// prepare mesh arrays for new vertices and faces
	mesh->vertices = (float *)RL_REALLOC(mesh->vertices, mesh->vertexCount*3*sizeof(float) + 24*3*sizeof(float));
	mesh->normals = (float *)RL_REALLOC(mesh->normals, mesh->vertexCount*3*sizeof(float) + 24*3*sizeof(float));
	mesh->indices = (unsigned short *)RL_REALLOC(mesh->indices, mesh->triangleCount*3*sizeof(unsigned short) + 12*3*sizeof(unsigned short));

	float vertices[72] = {0};
	float normals[72] = {0};
	unsigned short indices[36] = {0};
	
	//printf("lists:\n");
	//for (int i = 0; i < 36; i++) {
	//	printf("%f %f\n", vertices[i], normals[i]);
	//}
	//for (int i = 0; i < 18; i++) {
	//	printf("%d ", indices[i]);
	//}
	//printf("\n");
	// gen generic voxel
	// check if i should fetch front
	// gen front square
	fetchFront(mesh->vertexCount, vertices, normals, v_vectors, n_vectors, &voxel_v_count, &voxel_t_count, indices);
	fetchBack(mesh->vertexCount, vertices, normals, v_vectors, n_vectors, &voxel_v_count, &voxel_t_count, indices);
	fetchTop(mesh->vertexCount, vertices, normals, v_vectors, n_vectors, &voxel_v_count, &voxel_t_count, indices);
	fetchFloor(mesh->vertexCount, vertices, normals, v_vectors, n_vectors, &voxel_v_count, &voxel_t_count, indices);
	fetchRight(mesh->vertexCount, vertices, normals, v_vectors, n_vectors, &voxel_v_count, &voxel_t_count, indices);
	fetchLeft(mesh->vertexCount, vertices, normals, v_vectors, n_vectors, &voxel_v_count, &voxel_t_count, indices);

	// translate voxel
	for (int i = *(voxel_count) * 24; i < *(voxel_count) * 24 + 24; i++) {
		vertices[i*3 + 0] += pos_x;
		vertices[i*3 + 1] += pos_y;
		vertices[i*3 + 2] += pos_z;
	}

	for (int i = 0; i < 72; i++) {
		mesh->vertices[*(voxel_count) * 72 + i] = vertices[i];
		mesh->normals[*(voxel_count) * 72 + i] = normals[i];
	}
	
	for (int i = 0; i < 36; i++) {
		mesh->indices[*(voxel_count) * 36 + i] = indices[i];
	}
	
	mesh->vertexCount += voxel_v_count;
	mesh->triangleCount += voxel_t_count;
	*(voxel_count) += 1;


	printf("mesh:\n");
	for (int i = 0; i < 144; i++) {
		printf("%f %f\n", mesh->vertices[i], mesh->normals[i]);
	}
	for (int i = 0; i < 72; i++) {
		printf("%d ", mesh->indices[i]);
	}
	printf("\n");
	printf("vertex count: %d\n", mesh->vertexCount);
	printf("triangle count: %d\n", mesh->triangleCount);

	//mesh.vertices = (float *)RL_MALLOC(24*3*sizeof(float));
	//memcpy(mesh.vertices, vertices, 24*3*sizeof(float));

	//mesh.normals = (float *)RL_MALLOC(24*3*sizeof(float));
	//memcpy(mesh.normals, normals, 24*3*sizeof(float));

}

