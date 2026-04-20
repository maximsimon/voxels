// CREATING A VOXEL AT GIVEN POSITION - generates a mesh of a voxel (face by face), translates it to position and then add it to the current mesh

#ifndef FACES_H
#define FACES_H

#include "raylib.h"
#include "raymath.h"

// Generate a face of a voxel
void fetchFront(int vertex_count, float *vertices, float *normals, Vector3 *v_vectors, Vector3 *n_vectors, int *voxel_v_count, int *voxel_t_count, unsigned short *indices);
void fetchBack(int vertex_count, float *vertices, float *normals, Vector3 *v_vectors, Vector3 *n_vectors, int *voxel_v_count, int *voxel_t_count, unsigned short *indices);
void fetchTop(int vertex_count, float *vertices, float *normals, Vector3 *v_vectors, Vector3 *n_vectors, int *voxel_v_count, int *voxel_t_count, unsigned short *indices);
void fetchFloor(int vertex_count, float *vertices, float *normals, Vector3 *v_vectors, Vector3 *n_vectors, int *voxel_v_count, int *voxel_t_count, unsigned short *indices);
void fetchRight(int vertex_count, float *vertices, float *normals, Vector3 *v_vectors, Vector3 *n_vectors, int *voxel_v_count, int *voxel_t_count, unsigned short *indices);
void fetchLeft(int vertex_count, float *vertices, float *normals, Vector3 *v_vectors, Vector3 *n_vectors, int *voxel_v_count, int *voxel_t_count, unsigned short *indices);
void MeshVoxel(Mesh& mesh, float pos_x, float pos_y, float pos_z, float angle, int *voxel_count, int texture_type, float width, float height, float length);		// Generate a voxel at given position (pos_x, pos_y, pos_z), voxel_count is number of voxels already in mesh

#endif
