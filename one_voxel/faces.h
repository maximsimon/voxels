// CREATING MESHES OF EACH FACE OF A VOXEL

#ifndef FACES_H
#define FACES_H

#include "raylib.h"
#include "raymath.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void fetchFront(Mesh *mesh, float *vertices, float *normals, Vector3 *v_vectors, Vector3 *n_vectors, int *voxel_v_count, int *voxel_t_count, unsigned short *indices);	// gen fron square (2 triangles) of generic Voxel
void fetchBack(Mesh *mesh, float *vertices, float *normals, Vector3 *v_vectors, Vector3 *n_vectors, int *voxel_v_count, int *voxel_t_count, unsigned short *indices);
void fetchTop(Mesh *mesh, float *vertices, float *normals, Vector3 *v_vectors, Vector3 *n_vectors, int *voxel_v_count, int *voxel_t_count, unsigned short *indices);
void fetchFloor(Mesh *mesh, float *vertices, float *normals, Vector3 *v_vectors, Vector3 *n_vectors, int *voxel_v_count, int *voxel_t_count, unsigned short *indices);
void fetchRight(Mesh *mesh, float *vertices, float *normals, Vector3 *v_vectors, Vector3 *n_vectors, int *voxel_v_count, int *voxel_t_count, unsigned short *indices);
void fetchLeft(Mesh *mesh, float *vertices, float *normals, Vector3 *v_vectors, Vector3 *n_vectors, int *voxel_v_count, int *voxel_t_count, unsigned short *indices);

void fetchFront(Mesh *mesh, float *vertices, float *normals, Vector3 *v_vectors, Vector3 *n_vectors, int *voxel_v_count, int *voxel_t_count, unsigned short *indices) {
	
	// add 4 vertices (12 floats), 4 normals and increase voxel_v_count by 4

	vertices[0] = v_vectors[0].x;
	vertices[1] = v_vectors[0].y;
	vertices[2] = v_vectors[0].z;
	
	vertices[3] = v_vectors[1].x;
	vertices[4] = v_vectors[1].y;
	vertices[5] = v_vectors[1].z;
	
	vertices[6] = v_vectors[2].x;
	vertices[7] = v_vectors[2].y;
	vertices[8] = v_vectors[2].z;
	
	vertices[9] = v_vectors[3].x;
	vertices[10] = v_vectors[3].y;
	vertices[11] = v_vectors[3].z;


	normals[0] = n_vectors[0].x;
	normals[1] = n_vectors[0].y;
	normals[2] = n_vectors[0].z;
	
	normals[3] = n_vectors[0].x;
	normals[4] = n_vectors[0].y;
	normals[5] = n_vectors[0].z;
	
	normals[6] = n_vectors[0].x;
	normals[7] = n_vectors[0].y;
	normals[8] = n_vectors[0].z;
	
	normals[9] = n_vectors[0].x;
	normals[10] = n_vectors[0].y;
	normals[11] = n_vectors[0].z;


	indices[0] = 0;
	indices[1] = 1;
	indices[2] = 2;

	indices[3] = 0;
	indices[4] = 2;
	indices[5] = 3;
	
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

	vertices[12] = v_vectors[4].x;
	vertices[13] = v_vectors[4].y;
	vertices[14] = v_vectors[4].z;
	
	vertices[15] = v_vectors[5].x;
	vertices[16] = v_vectors[5].y;
	vertices[17] = v_vectors[5].z;
	
	vertices[18] = v_vectors[6].x;
	vertices[19] = v_vectors[6].y;
	vertices[20] = v_vectors[6].z;
	
	vertices[21] = v_vectors[7].x;
	vertices[22] = v_vectors[7].y;
	vertices[23] = v_vectors[7].z;


	normals[12] = n_vectors[1].x;
	normals[13] = n_vectors[1].y;
	normals[14] = n_vectors[1].z;
	
	normals[15] = n_vectors[1].x;
	normals[16] = n_vectors[1].y;
	normals[17] = n_vectors[1].z;
	
	normals[18] = n_vectors[1].x;
	normals[19] = n_vectors[1].y;
	normals[20] = n_vectors[1].z;
	
	normals[21] = n_vectors[1].x;
	normals[22] = n_vectors[1].y;
	normals[23] = n_vectors[1].z;

	
	indices[6] = 4;
	indices[7] = 5;
	indices[8] = 6;

	indices[9] = 4;
	indices[10] = 6;
	indices[11] = 7;
	
	*(voxel_t_count) += 2;
	*(voxel_v_count) += 4;
	
	printf("back vertices:\n");
	for (int i = 12; i < 24; i++) {
		printf("%f %f\n", vertices[i], normals[i]);
	}
	for (int i = 6; i < 12; i++) {
		printf("%d ", indices[i]);
	}
	printf("\n");	
	// apply new stuff to mesh
	for (int i = 12; i < 24; i++) {
		mesh->vertices[i] = vertices[i];
		mesh->normals[i] = normals[i];
	}
	for (int i = 6; i < 12; i++) {
		mesh->indices[i] = indices[i];
	}
		
	mesh->vertexCount = *(voxel_v_count);
	mesh->triangleCount = *(voxel_t_count);
	
	printf("back mesh:\n");
	for (int i = 12; i < 24; i++) {
		printf("%f %f\n", mesh->vertices[i], mesh->normals[i]);
	}
	for (int i = 6; i < 12; i++) {
		printf("%d ", mesh->indices[i]);
	}
	printf("\n");	
	printf("vertex count: %d\n", mesh->vertexCount);
	printf("triangle count: %d\n", mesh->triangleCount);

}

void fetchTop(Mesh *mesh, float *vertices, float *normals, Vector3 *v_vectors, Vector3 *n_vectors, int *voxel_v_count, int *voxel_t_count, unsigned short *indices) {
	// add 4 vertices (12 floats), 4 normals and increase voxel_v_count by 4

	vertices[24] = v_vectors[5].x;
	vertices[25] = v_vectors[5].y;
	vertices[26] = v_vectors[5].z;
	
	vertices[27] = v_vectors[3].x;
	vertices[28] = v_vectors[3].y;
	vertices[29] = v_vectors[3].z;
	
	vertices[30] = v_vectors[2].x;
	vertices[31] = v_vectors[2].y;
	vertices[32] = v_vectors[2].z;
	
	vertices[33] = v_vectors[6].x;
	vertices[34] = v_vectors[6].y;
	vertices[35] = v_vectors[6].z;


	normals[24] = n_vectors[2].x;
	normals[25] = n_vectors[2].y;
	normals[26] = n_vectors[2].z;
	
	normals[27] = n_vectors[2].x;
	normals[28] = n_vectors[2].y;
	normals[29] = n_vectors[2].z;
	
	normals[30] = n_vectors[2].x;
	normals[31] = n_vectors[2].y;
	normals[32] = n_vectors[2].z;
	
	normals[33] = n_vectors[2].x;
	normals[34] = n_vectors[2].y;
	normals[35] = n_vectors[2].z;

	
	indices[12] = 8;
	indices[13] = 9;
	indices[14] = 10;

	indices[15] = 8;
	indices[16] = 10;
	indices[17] = 11;
	
	*(voxel_t_count) += 2;
	*(voxel_v_count) += 4;
	
	printf("ceiling vertices:\n");
	for (int i = 24; i < 36; i++) {
		printf("%f %f\n", vertices[i], normals[i]);
	}
	for (int i = 12; i < 18; i++) {
		printf("%d ", indices[i]);
	}
	printf("\n");	
	// apply new stuff to mesh
	for (int i = 24; i < 36; i++) {
		mesh->vertices[i] = vertices[i];
		mesh->normals[i] = normals[i];
	}
	for (int i = 12; i < 18; i++) {
		mesh->indices[i] = indices[i];
	}
		
	mesh->vertexCount = *(voxel_v_count);
	mesh->triangleCount = *(voxel_t_count);
	
	printf("ceiling mesh:\n");
	for (int i = 24; i < 36; i++) {
		printf("%f %f\n", mesh->vertices[i], mesh->normals[i]);
	}
	for (int i = 12; i < 18; i++) {
		printf("%d ", mesh->indices[i]);
	}
	printf("\n");	
	printf("vertex count: %d\n", mesh->vertexCount);
	printf("triangle count: %d\n", mesh->triangleCount);

}

void fetchFloor(Mesh *mesh, float *vertices, float *normals, Vector3 *v_vectors, Vector3 *n_vectors, int *voxel_v_count, int *voxel_t_count, unsigned short *indices) {
	// add 4 vertices (12 floats), 4 normals and increase voxel_v_count by 4

	vertices[36] = v_vectors[4].x;
	vertices[37] = v_vectors[4].y;
	vertices[38] = v_vectors[4].z;
	
	vertices[39] = v_vectors[7].x;
	vertices[40] = v_vectors[7].y;
	vertices[41] = v_vectors[7].z;
	
	vertices[42] = v_vectors[1].x;
	vertices[43] = v_vectors[1].y;
	vertices[44] = v_vectors[1].z;
	
	vertices[45] = v_vectors[0].x;
	vertices[46] = v_vectors[0].y;
	vertices[47] = v_vectors[0].z;


	normals[36] = n_vectors[3].x;
	normals[37] = n_vectors[3].y;
	normals[38] = n_vectors[3].z;
	
	normals[39] = n_vectors[3].x;
	normals[40] = n_vectors[3].y;
	normals[41] = n_vectors[3].z;
	
	normals[42] = n_vectors[3].x;
	normals[43] = n_vectors[3].y;
	normals[44] = n_vectors[3].z;
	
	normals[45] = n_vectors[3].x;
	normals[46] = n_vectors[3].y;
	normals[47] = n_vectors[3].z;

	
	indices[18] = 12;
	indices[19] = 13;
	indices[20] = 14;

	indices[21] = 12;
	indices[22] = 14;
	indices[23] = 15;
	
	*(voxel_t_count) += 2;
	*(voxel_v_count) += 4;
	
	// apply new stuff to mesh
	for (int i = 36; i < 48; i++) {
		mesh->vertices[i] = vertices[i];
		mesh->normals[i] = normals[i];
	}
	for (int i = 18; i < 24; i++) {
		mesh->indices[i] = indices[i];
	}
		
	mesh->vertexCount = *(voxel_v_count);
	mesh->triangleCount = *(voxel_t_count);

}

void fetchRight(Mesh *mesh, float *vertices, float *normals, Vector3 *v_vectors, Vector3 *n_vectors, int *voxel_v_count, int *voxel_t_count, unsigned short *indices) {
	// add 4 vertices (12 floats), 4 normals and increase voxel_v_count by 4

	vertices[48] = v_vectors[1].x;
	vertices[49] = v_vectors[1].y;
	vertices[50] = v_vectors[1].z;
	
	vertices[51] = v_vectors[7].x;
	vertices[52] = v_vectors[7].y;
	vertices[53] = v_vectors[7].z;
	
	vertices[54] = v_vectors[6].x;
	vertices[55] = v_vectors[6].y;
	vertices[56] = v_vectors[6].z;
	
	vertices[57] = v_vectors[2].x;
	vertices[58] = v_vectors[2].y;
	vertices[59] = v_vectors[2].z;


	normals[48] = n_vectors[4].x;
	normals[49] = n_vectors[4].y;
	normals[50] = n_vectors[4].z;
	
	normals[51] = n_vectors[4].x;
	normals[52] = n_vectors[4].y;
	normals[53] = n_vectors[4].z;
	
	normals[54] = n_vectors[4].x;
	normals[55] = n_vectors[4].y;
	normals[56] = n_vectors[4].z;
	
	normals[57] = n_vectors[4].x;
	normals[58] = n_vectors[4].y;
	normals[59] = n_vectors[4].z;

	
	indices[24] = 16;
	indices[25] = 17;
	indices[26] = 18;

	indices[27] = 16;
	indices[28] = 18;
	indices[29] = 19;
	
	*(voxel_t_count) += 2;
	*(voxel_v_count) += 4;
	
	// apply new stuff to mesh
	for (int i = 48; i < 60; i++) {
		mesh->vertices[i] = vertices[i];
		mesh->normals[i] = normals[i];
	}
	for (int i = 24; i < 30; i++) {
		mesh->indices[i] = indices[i];
	}
		
	mesh->vertexCount = *(voxel_v_count);
	mesh->triangleCount = *(voxel_t_count);

}

void fetchLeft(Mesh *mesh, float *vertices, float *normals, Vector3 *v_vectors, Vector3 *n_vectors, int *voxel_v_count, int *voxel_t_count, unsigned short *indices) {
	// add 4 vertices (12 floats), 4 normals and increase voxel_v_count by 4

	vertices[60] = v_vectors[4].x;
	vertices[61] = v_vectors[4].y;
	vertices[62] = v_vectors[4].z;
	
	vertices[63] = v_vectors[0].x;
	vertices[64] = v_vectors[0].y;
	vertices[65] = v_vectors[0].z;
	
	vertices[66] = v_vectors[3].x;
	vertices[67] = v_vectors[3].y;
	vertices[68] = v_vectors[3].z;
	
	vertices[69] = v_vectors[5].x;
	vertices[70] = v_vectors[5].y;
	vertices[71] = v_vectors[5].z;


	normals[60] = n_vectors[5].x;
	normals[61] = n_vectors[5].y;
	normals[62] = n_vectors[5].z;
	
	normals[63] = n_vectors[5].x;
	normals[64] = n_vectors[5].y;
	normals[65] = n_vectors[5].z;
	
	normals[66] = n_vectors[5].x;
	normals[67] = n_vectors[5].y;
	normals[68] = n_vectors[5].z;
	
	normals[69] = n_vectors[5].x;
	normals[70] = n_vectors[5].y;
	normals[71] = n_vectors[5].z;

	
	indices[30] = 20;
	indices[31] = 21;
	indices[32] = 22;

	indices[33] = 20;
	indices[34] = 22;
	indices[35] = 23;
	
	*(voxel_t_count) += 2;
	*(voxel_v_count) += 4;
	
	// apply new stuff to mesh
	for (int i = 60; i < 72; i++) {
		mesh->vertices[i] = vertices[i];
		mesh->normals[i] = normals[i];
	}
	for (int i = 30; i < 36; i++) {
		mesh->indices[i] = indices[i];
	}
		
	mesh->vertexCount = *(voxel_v_count);
	mesh->triangleCount = *(voxel_t_count);

}

#endif

