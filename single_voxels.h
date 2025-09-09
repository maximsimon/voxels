// GENERATE MESH OF MAZE FROM BLACK AND WHITE IMAGE OF THE MAZE MAP, image and mesh map are 1 px to 1 unit ratio, there is some compressing of drawing to 1 to 1 ratio walls : empty for increased ridgitness of Labyrinth

#ifndef SINGLE_VOXELS_H
#define SINGLE_VOXELS_H
	
#define GRAY_VALUE(c) ((float)(c.r + c.g + c.b)/3.0f)

#include "raylib.h"
#include "raymath.h"
#include <stdio.h>
#include "rlgl.h"           // OpenGL abstraction layer to OpenGL 1.1, 2.1, 3.3+ or ES2
#include <string.h>
#include <stdlib.h>

Mesh MeshCube(Mesh *maze_mesh, float x, float y, float z, float pos_x, float pos_y, float pos_z); 		// add a cube at desired position to a mesh
Mesh MeshVoxel(Mesh *maze_mesh, float pos_x, float pos_y, float pos_z); 		// add a voxel at desired position to a mesh
Mesh OGMeshCube(float width, float height, float length);		// template from raylib library

Mesh OGMeshCube(float width, float height, float length)
{
    Mesh mesh = { 0 };

    float vertices[] = {
        -width/2, -height/2, length/2,
        width/2, -height/2, length/2,
        width/2, height/2, length/2,
        -width/2, height/2, length/2,
        -width/2, -height/2, -length/2,
        -width/2, height/2, -length/2,
        width/2, height/2, -length/2,
        width/2, -height/2, -length/2,
        -width/2, height/2, -length/2,
        -width/2, height/2, length/2,
        width/2, height/2, length/2,
        width/2, height/2, -length/2,
        -width/2, -height/2, -length/2,
        width/2, -height/2, -length/2,
        width/2, -height/2, length/2,
        -width/2, -height/2, length/2,
        width/2, -height/2, -length/2,
        width/2, height/2, -length/2,
        width/2, height/2, length/2,
        width/2, -height/2, length/2,
        -width/2, -height/2, -length/2,
        -width/2, -height/2, length/2,
        -width/2, height/2, length/2,
        -width/2, height/2, -length/2
    };

    float texcoords[] = {
        0.0f, 0.0f,
        1.0f, 0.0f,
        1.0f, 1.0f,
        0.0f, 1.0f,
        1.0f, 0.0f,
        1.0f, 1.0f,
        0.0f, 1.0f,
        0.0f, 0.0f,
        0.0f, 1.0f,
        0.0f, 0.0f,
        1.0f, 0.0f,
        1.0f, 1.0f,
        1.0f, 1.0f,
        0.0f, 1.0f,
        0.0f, 0.0f,
        1.0f, 0.0f,
        1.0f, 0.0f,
        1.0f, 1.0f,
        0.0f, 1.0f,
        0.0f, 0.0f,
        0.0f, 0.0f,
        1.0f, 0.0f,
        1.0f, 1.0f,
        0.0f, 1.0f
    };

    float normals[] = {
        0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f,
        0.0f, 0.0f,-1.0f,
        0.0f, 0.0f,-1.0f,
        0.0f, 0.0f,-1.0f,
        0.0f, 0.0f,-1.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f,-1.0f, 0.0f,
        0.0f,-1.0f, 0.0f,
        0.0f,-1.0f, 0.0f,
        0.0f,-1.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        -1.0f, 0.0f, 0.0f,
        -1.0f, 0.0f, 0.0f,
        -1.0f, 0.0f, 0.0f,
        -1.0f, 0.0f, 0.0f
    };

    mesh.vertices = (float *)RL_MALLOC(24*3*sizeof(float));
    memcpy(mesh.vertices, vertices, 24*3*sizeof(float));

    mesh.texcoords = (float *)RL_MALLOC(24*2*sizeof(float));
    memcpy(mesh.texcoords, texcoords, 24*2*sizeof(float));

    mesh.normals = (float *)RL_MALLOC(24*3*sizeof(float));
    memcpy(mesh.normals, normals, 24*3*sizeof(float));

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

	for (int i = 0; i < 36; i++) {
		printf("%d ", mesh.indices[i]);
	} printf("\n");

    // Upload vertex data to GPU (static mesh)
    //UploadMesh(&mesh, false);

    return mesh;
}

Mesh MeshVoxel(Mesh *maze_mesh, float pos_x, float pos_y, float pos_z) {
	return MeshCube(maze_mesh, 1.0f, 1.0f, 1.0f, pos_x, pos_y, pos_z);
}

Mesh MeshCube(Mesh *maze_mesh, float x, float y, float z, float pos_x, float pos_y, float pos_z) {

	printf("x, y, z: %f %f %f \n", z, y, z);
	// 24 verticies in perticuliar order
	// 0 1 2 	0 2 3	 4 5 6	 4 6 7	 8 9 10		8 10 11 	12 13 14	 12 14 15	 16 17 18	 16 18 19	 20 21 22	 20 22 23 
	//it goes, front, left, back, right, floor, ceiling
	float vertices[] = {
		-x/2, -y/2, z/2,	//0 front
		x/2, -y/2, z/2,		//1 front
		x/2, y/2, z/2,		//2 front

		-x/2, -y/2, z/2,	//0 front
		x/2, y/2, z/2,		//2, front
		-x/2, y/2, z/2,		//3, front

		-x/2, -y/2, z/2,	//0, left
		-x/2, -y/2, -z/2,	//7, left
		-x/2, y/2, -z/2,	//3, left	.....

		-x/2, -y/2, z/2,
		-x/2, y/2, z/2,
		-x/2, y/2, -z/2,

		x/2, -y/2, -z/2,
		-x/2, -y/2, -z/2,
		-x/2, y/2, -z/2,
		
		x/2, -y/2, -z/2,
		-x/2, y/2, -z/2,
		x/2, y/2, -z/2,

		x/2, -y/2, -z/2,
		x/2, -y/2, z/2,
		x/2, y/2, z/2,	

		x/2, -y/2, -z/2,
		x/2, -y/2, z/2,
		x/2, y/2, z/2,

		-x/2, -y/2, z/2,
		x/2, -y/2, z/2,
		x/2, -y/2, -z/2,

		-x/2, -y/2, z/2,
		x/2, -y/2, -z/2,
		-x/2, -y/2, -z/2,

		x/2, y/2, z/2,
		-z/2, y/2, z/2,
		-x/2, y/2, -z/2,
		
		x/2, y/2, z/2,
		-x/2, y/2, -z/2,
		x/2, y/2, -z/2
	};		// 36 elements - cube - 6 faces, 12 triangles, 36 vertices (each triangle 3 vertices always)

	// normals go outside of the cube
    float normals[] = {
        0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f,
        -1.0f, 0.0f, 0.0f,
        -1.0f, 0.0f, 0.0f,
        -1.0f, 0.0f, 0.0f,
        -1.0f, 0.0f, 0.0f,
        -1.0f, 0.0f, 0.0f,
        -1.0f, 0.0f, 0.0f,
        0.0f, 0.0f,-1.0f,
        0.0f, 0.0f,-1.0f,
        0.0f, 0.0f,-1.0f,
        0.0f, 0.0f,-1.0f,
        0.0f, 0.0f,-1.0f,
        0.0f, 0.0f,-1.0f,
        1.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        0.0f,-1.0f, 0.0f,
        0.0f,-1.0f, 0.0f,
        0.0f,-1.0f, 0.0f,
        0.0f,-1.0f, 0.0f,
        0.0f,-1.0f, 0.0f,
        0.0f,-1.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f
    };


	
	
	//memcpy(maze_mesh->vertices, vertices, 2 * 24 * 3 * sizeof(float));
	//memcpy(maze_mesh->vertices + 24 * 3 * sizeof(float), vertices, 24 * 3 * sizeof(float));
	//memcpy(maze_mesh->texcoords, texcoords, 8 * 2 * sizeof(float));

	/*
    	for (int i = 0; i < 24; i++) {
		maze_mesh->vertices[i] = vertices[i];
		maze_mesh->normals[i] = normals[i];
		printf(" %f", maze_mesh->normals[i]);
		printf(" %f", maze_mesh->vertices[i]);
		printf("\n");
	}
	*/
	
	for (int i = 0; i < 6; i++) {
		vertices[i*3 + 0] += pos_x;
		vertices[i*3 + 1] += pos_y;
		vertices[i*3 + 2] += pos_z;
	}

	maze_mesh->vertices[0] = vertices[0];
	maze_mesh->vertices[1] = vertices[1];
	maze_mesh->vertices[2] = vertices[2];

	maze_mesh->vertices[3] = vertices[3];
	maze_mesh->vertices[4] = vertices[4];
	maze_mesh->vertices[5] = vertices[5];
	
	maze_mesh->vertices[6] = vertices[6];
	maze_mesh->vertices[7] = vertices[7];
	maze_mesh->vertices[8] = vertices[8];

	maze_mesh->vertices[9] = vertices[9];
	maze_mesh->vertices[10] = vertices[10];
	maze_mesh->vertices[11] = vertices[11];

	maze_mesh->vertices[12] = vertices[12];
	maze_mesh->vertices[13] = vertices[13];
	maze_mesh->vertices[14] = vertices[14];
	
	maze_mesh->vertices[15] = vertices[15];
	maze_mesh->vertices[16] = vertices[16];
	maze_mesh->vertices[17] = vertices[17];
	
	maze_mesh->normals[0] = normals[0];
	maze_mesh->normals[1] = normals[1];
	maze_mesh->normals[2] = normals[2];

	maze_mesh->normals[3] = normals[3];
	maze_mesh->normals[4] = normals[4];
	maze_mesh->normals[5] = normals[5];
	
	maze_mesh->normals[6] = normals[6];
	maze_mesh->normals[7] = normals[7];
	maze_mesh->normals[8] = normals[8];
	
	maze_mesh->normals[9] = normals[9];
	maze_mesh->normals[10] = normals[10];
	maze_mesh->normals[11] = normals[11];

	maze_mesh->normals[12] = normals[12];
	maze_mesh->normals[13] = normals[13];
	maze_mesh->normals[14] = normals[14];
	
	maze_mesh->normals[15] = normals[15];
	maze_mesh->normals[16] = normals[16];
	maze_mesh->normals[17] = normals[17];
	
	maze_mesh->indices[0] = 0;
	maze_mesh->indices[1] = 1;
	maze_mesh->indices[2] = 2;
	
	maze_mesh->indices[3] = 0;
	maze_mesh->indices[4] = 2;
	maze_mesh->indices[5] = 3;

	
	/*
	int k = 0;
	// Indices can be initialized right now
	for (int i = 0; i < 36; i += 6)
	{
		maze_mesh->indices[i] = 4*k;
		maze_mesh->indices[i + 1] = 4*k + 1;
		maze_mesh->indices[i + 2] = 4*k + 2;
		maze_mesh->indices[i + 3] = 4*k;
		maze_mesh->indices[i + 4] = 4*k + 2;
		maze_mesh->indices[i + 5] = 4*k + 3;

		k++;
	}
	*/

	printf("vertices\n");
	for (int i = 0; i < 18; i++) {
		printf("%f ", maze_mesh->vertices[i]);
	}
	printf("\n");
	printf("normals\n");
	for (int i = 0; i < 18; i++) {
		printf("%f ", maze_mesh->normals[i]);
	}
	printf("\n");
	printf("indices\n");
	for (int i = 0; i < 6; i++) {
		printf("%d ", maze_mesh->indices[i]);
	}
	printf("\n");
	
	maze_mesh->vertexCount += 6;
	maze_mesh->triangleCount += 2;
	//maze_mesh->vertexCount += 24;
	//maze_mesh->triangleCount += 12;
	return *maze_mesh;
}

#endif
