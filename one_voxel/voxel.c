// VOXELS: PROGRAMMING MY RENDERING OF VOXEL WORLD, starting with a MAZE FROM PICTURE AND MY KEYBINDINGS FOR MOVEMENT etc

#include "raylib.h"
#include <stdio.h>
#include "camera_movement.h"
//#include "meshes.h"
#include <stdlib.h>
#include <string.h>

const int screen_width = 1600;
const int screen_height = 850;

Mesh MeshCube(float width, float height, float length, float pos_x, float pos_y, float pos_z);	// generate one cube at the origin
void fetchFront(Mesh *mesh, float *vertices, float *normals, Vector3 *v_vectors, Vector3 *n_vectors, int *voxel_v_count, int *voxel_t_count, unsigned short *indices);	// gen fron square (2 triangles) of generic Voxel
void fetchBack(Mesh *mesh, float *vertices, float *normals, Vector3 *v_vectors, Vector3 *n_vectors, int *voxel_v_count, int *voxel_t_count, unsigned short *indices);

int main(void){

	InitWindow(screen_width, screen_height, "You're in voxels now.");
	SetTargetFPS(60);
	
	Camera camera = { 0 };
	camera.position = (Vector3){ 1.0f, 1.0f, 1.0f };    // Camera position
	camera.target = (Vector3){ 0.0f, 1.0f, 0.0f };    // Camera looking at point
	camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };          // Camera up vector (rotation towards target)
	camera.fovy = 45.0f;                                // Camera field-of-view Y
	camera.projection = CAMERA_PERSPECTIVE;             // Camera projection type

	Mesh mesh = MeshCube(1.0f, 1.0f, 1.0f, -5.0f, 2.0f, -5.0f);

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

Mesh MeshCube(float width, float height, float length, float pos_x, float pos_y, float pos_z) {

	Mesh mesh = { 0 };
    
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

	mesh.vertices = (float *)RL_MALLOC(4*3*sizeof(float));
	mesh.normals = (float *)RL_MALLOC(4*3*sizeof(float));
	mesh.indices = (unsigned short *)RL_MALLOC(2*3*sizeof(unsigned short));
	
	float vertices[24] = {0};
	float normals[24] = {0};
	unsigned short indices[6] = {0};

	// gen generic voxel
	// check if i should fetch front
	// gen front square
	fetchFront(&mesh, vertices, normals, v_vectors, n_vectors, &voxel_v_count, &voxel_t_count, indices);
	//fetchBack(&mesh, vertices, normals, v_vectors, n_vectors, &voxel_v_count, &voxel_t_count, indices);

	// translate voxel
	for (int i = 0; i < 4; i++) {
		mesh.vertices[i*3 + 0] += pos_x;
		mesh.vertices[i*3 + 1] += pos_y;
		mesh.vertices[i*3 + 2] += pos_z;
	}
	
	for (int i = 0; i < 12; i++) {
		printf("%f %f\n", mesh.vertices[i], mesh.normals[i]);
	}
	for (int i = 0; i < 6; i++) {
		printf("%d ", mesh.indices[i]);
	}
	printf("\n");
	printf("vertex count: %d\n", mesh.vertexCount);
	printf("triangle count: %d\n", mesh.triangleCount);

	//mesh.vertices = (float *)RL_MALLOC(24*3*sizeof(float));
	//memcpy(mesh.vertices, vertices, 24*3*sizeof(float));

	//mesh.normals = (float *)RL_MALLOC(24*3*sizeof(float));
	//memcpy(mesh.normals, normals, 24*3*sizeof(float));


	/*
	mesh.indices = (unsigned short *)RL_MALLOC(36*sizeof(unsigned short));

	int k = 0;

	// Indices can be initialized right now
	for (int i = 0; i < 36; i += 6)
	{
	    mesh.indices[i] = 4*k;
	    mesh.indices[i + 1] = 4*k + 1;
	    mesh.indices[i + 2] = 4*k + 2;
	    mesh.indices[i + 3] = 4*k;
	    mesh.indices[i + 4] = 4*k + 2;
	    mesh.indices[i + 5] = 4*k + 3;

	    k++;
	}

	mesh.vertexCount = 24;
	mesh.triangleCount = 12;
	*/

	return mesh;
}

void fetchFront(Mesh *mesh, float *vertices, float *normals, Vector3 *v_vectors, Vector3 *n_vectors, int *voxel_v_count, int *voxel_t_count, unsigned short *indices) {
	
	// add 4 vertices (12 floats), 4 normals and increase voxel_v_count by 4

	vertices[*(voxel_v_count)*3 + 0] = v_vectors[0].x;
	vertices[*(voxel_v_count)*3+ 1] = v_vectors[0].y;
	vertices[*(voxel_v_count)*3+ 2] = v_vectors[0].z;
	
	vertices[*(voxel_v_count)*3+ 3] = v_vectors[1].x;
	vertices[*(voxel_v_count)*3+ 4] = v_vectors[1].y;
	vertices[*(voxel_v_count)*3+ 5] = v_vectors[1].z;
	
	vertices[*(voxel_v_count)*3+ 6] = v_vectors[2].x;
	vertices[*(voxel_v_count)*3+ 7] = v_vectors[2].y;
	vertices[*(voxel_v_count)*3+ 8] = v_vectors[2].z;
	
	vertices[*(voxel_v_count)*3+ 9] = v_vectors[3].x;
	vertices[*(voxel_v_count)*3+ 10] = v_vectors[3].y;
	vertices[*(voxel_v_count)*3+ 11] = v_vectors[3].z;


	normals[*(voxel_v_count)*3 + 0] = n_vectors[0].x;
	normals[*(voxel_v_count)*3 + 1] = n_vectors[0].y;
	normals[*(voxel_v_count)*3 + 2] = n_vectors[0].z;
	
	normals[*(voxel_v_count)*3 + 3] = n_vectors[0].x;
	normals[*(voxel_v_count)*3 + 4] = n_vectors[0].y;
	normals[*(voxel_v_count)*3 + 5] = n_vectors[0].z;
	
	normals[*(voxel_v_count)*3 + 6] = n_vectors[0].x;
	normals[*(voxel_v_count)*3 + 7] = n_vectors[0].y;
	normals[*(voxel_v_count)*3 + 8] = n_vectors[0].z;
	
	normals[*(voxel_v_count)*3 + 9] = n_vectors[0].x;
	normals[*(voxel_v_count)*3 + 10] = n_vectors[0].y;
	normals[*(voxel_v_count)*3 + 11] = n_vectors[0].z;


	indices[*(voxel_t_count)*3 + 0] = 0;
	indices[*(voxel_t_count)*3 + 1] = 1;
	indices[*(voxel_t_count)*3 + 2] = 2;

	indices[*(voxel_t_count)*3 + 3] = 0;
	indices[*(voxel_t_count)*3 + 4] = 2;
	indices[*(voxel_t_count)*3 + 5] = 3;
	
	*(voxel_t_count) += 2;
	*(voxel_v_count) += 4;
	
	
	// apply new stuff to mesh
	for (int i = 0; i < 12; i++) {
		mesh->vertices[i] = vertices[i];
		mesh->normals[i] = normals[i];
	}
	for (int i = 0; i < 6; i++) {
		mesh->indices[i] = indices[i];
	}
		
	mesh->vertexCount = *(voxel_v_count);
	mesh->triangleCount = *(voxel_t_count);

}

void fetchBack(Mesh *mesh, float *vertices, float *normals, Vector3 *v_vectors, Vector3 *n_vectors, int *voxel_v_count, int *voxel_t_count, unsigned short *indices) {
	// add 4 vertices (12 floats), 4 normals and increase voxel_v_count by 4

	vertices[*(voxel_v_count)*3 + 0] = v_vectors[4].x;
	vertices[*(voxel_v_count)*3+ 1] = v_vectors[4].y;
	vertices[*(voxel_v_count)*3+ 2] = v_vectors[4].z;
	
	vertices[*(voxel_v_count)*3+ 3] = v_vectors[5].x;
	vertices[*(voxel_v_count)*3+ 4] = v_vectors[5].y;
	vertices[*(voxel_v_count)*3+ 5] = v_vectors[5].z;
	
	vertices[*(voxel_v_count)*3+ 6] = v_vectors[6].x;
	vertices[*(voxel_v_count)*3+ 7] = v_vectors[6].y;
	vertices[*(voxel_v_count)*3+ 8] = v_vectors[6].z;
	
	vertices[*(voxel_v_count)*3+ 9] = v_vectors[7].x;
	vertices[*(voxel_v_count)*3+ 10] = v_vectors[7].y;
	vertices[*(voxel_v_count)*3+ 11] = v_vectors[7].z;


	normals[*(voxel_v_count)*3 + 0] = n_vectors[0].x;
	normals[*(voxel_v_count)*3 + 1] = n_vectors[0].y;
	normals[*(voxel_v_count)*3 + 2] = n_vectors[0].z;
	
	normals[*(voxel_v_count)*3 + 3] = n_vectors[0].x;
	normals[*(voxel_v_count)*3 + 4] = n_vectors[0].y;
	normals[*(voxel_v_count)*3 + 5] = n_vectors[0].z;
	
	normals[*(voxel_v_count)*3 + 6] = n_vectors[0].x;
	normals[*(voxel_v_count)*3 + 7] = n_vectors[0].y;
	normals[*(voxel_v_count)*3 + 8] = n_vectors[0].z;
	
	normals[*(voxel_v_count)*3 + 9] = n_vectors[0].x;
	normals[*(voxel_v_count)*3 + 10] = n_vectors[0].y;
	normals[*(voxel_v_count)*3 + 11] = n_vectors[0].z;


	indices[*(voxel_t_count)*3 + 0] = 4;
	indices[*(voxel_t_count)*3 + 1] = 5;
	indices[*(voxel_t_count)*3 + 2] = 6;

	indices[*(voxel_t_count)*3 + 3] = 4;
	indices[*(voxel_t_count)*3 + 4] = 6;
	indices[*(voxel_t_count)*3 + 5] = 7;
	
	*(voxel_t_count) += 2;
	*(voxel_v_count) += 4;
	
	
	// apply new stuff to mesh
	for (int i = 0; i < 12; i++) {
		mesh->vertices[i] = vertices[i];
		mesh->normals[i] = normals[i];
	}
	for (int i = 0; i < 6; i++) {
		mesh->indices[i] = indices[i];
	}
		
	mesh->vertexCount = *(voxel_v_count);
	mesh->triangleCount = *(voxel_t_count);

}
