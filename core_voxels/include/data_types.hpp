// DECLARATIONS OF CUSTOM TYPEDEF STRUCTS, DATATYPES etc USED TROUGHOUT VOXEL WORLD

#ifndef DATA_TYPES_H
#define DATA_TYPES_H

#include "raylib.h"
#include "raymath.h"

#include <opencv2/opencv.hpp>

#include <GL/glew.h>
#include <GL/gl.h>          // OpenGL core functions
#include <GL/glext.h>       // OpenGL extensions (for PBO)

#define GRAY_VALUE(c) ((float)(c.r + c.g + c.b)/3.0f)

// window size
extern const int screen_width;
extern const int screen_height;

// chunk size
extern const int chunk_w;
extern const int chunk_h;

// actual core map of the Voxel World (1 is voxel 0 is nothing)
typedef struct {
    int map[16 * 16];
} chunkMap;

// entire map
typedef struct {
	chunkMap *chunks;
	int width_px;		// width of entire world in pixels
	int height_px;		// height of entire world in pixels
	int width_chunks; 		// height in chunks
	int height_chunks;		// width of entire world in chunks
	int chunk_side;			// size one side of chunk in pixels (chunks are squares)
} mainMap;

typedef struct {
	cv::Mat camera_front;		// lamst POV view of front camera of the robot in simulation
} Observation;

typedef struct {
	Vector3 linear_vel;
	Vector3 angular_vel;
} Action;

typedef struct VoxelWorld {
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
	RenderTexture2D camera_view_tex;
} VoxelWorld;


#endif
