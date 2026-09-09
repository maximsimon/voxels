// DECLARATIONS OF CUSTOM TYPEDEF STRUCTS, DATATYPES etc USED TROUGHOUT VOXEL WORLD

#ifndef DATA_TYPES_H
#define DATA_TYPES_H

#include "raylib.h"
#include "raymath.h"

#include <opencv2/opencv.hpp>
#include <vector>
#include <unordered_map>

#include "config_core.hpp"

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
    int voxel_count;
    Vector3 chunk_position;
} chunkMap;

// convinience for working with the maze map
typedef struct {
	int chunk;		// index of a chunk
	int cell;		// index of a cell inside a chunk
} mapCoords;

// entire map
typedef struct {
	chunkMap *chunks;
	int width_px;		// width of entire world in pixels
	int height_px;		// height of entire world in pixels
	int width_chunks; 		// height in chunks
	int height_chunks;		// width of entire world in chunks
	int chunk_side;			// size one side of chunk in pixels (chunks are squares)
	mapCoords map_coords;
} mainMap;


typedef struct {
	cv::Mat camera_front;		// last POV view of front camera of the robot in simulation
	Vector3 position;
	Quaternion orientation;
	Vector3 linear_vel;
	Vector3 angular_vel;
	// heading in the floor plane measured from +x, radians (getPlayerAngle convention).
	// Carried explicitly because recovering it from `orientation` means undoing raylib's
	// camera-forward offset - see rl_quat_to_ros in the ros2_api branch for how easy that
	// is to get wrong.
	float yaw;
	float lidar_scan[NUM_LIDAR_RAYS];
} Observation;

typedef struct {
	Vector3 linear_vel;
	Vector3 angular_vel;
} Action;

// teleport input box (activated by T key)
// text was a flexible array member, but teleportText is embedded by value in VoxelWorld,
// so every write into it ran past the end of the VoxelWorld allocation.  Give it a real
// buffer: capacity is MAX_INPUT_CHARS plus room for the null terminator.
#define TELEPORT_TEXT_CAPACITY 32

typedef struct {
	bool text_active;
	int letter_count;
	Rectangle text_box;
	int MAX_INPUT_CHARS;
	char text[TELEPORT_TEXT_CAPACITY];
} teleportText;

typedef struct VoxelWorld {
	bool player_view;		// controls if player or edit POV is viewed
	bool player_mode;		// controls if arrow keys control player voxel or flying edit camera
	Camera3D edit_camera;		// flying "god mode" camera
	Camera3D player_camera;		// player (robot) camera
	Camera3D current_camera;	// player or edit camera - it is the one that is currently being rendered
	mainMap main_map;		// array map of the world
	Model player_model;		// 1 red voxel - model of the player (robot)
	float player_angle;
	RenderTexture2D camera_view_tex;
	Model *maze_model;		// model of the world
	Mesh *maze_mesh;			// mesh of the world;
	int maze_chunk_count;		// number of entries in maze_model / maze_mesh
	Model ground_model;		// model for texturing the ground visually
	Model sky_model;
	// textures owned by the world - kept so a world swap can unload them instead of leaking them on the GPU
	Texture2D atlas_texture;
	Texture2D ground_texture;
	Texture2D sky_texture;
	bool world_built;		// false before the first build_world(), false again after destroy_world()
	World current_world;
	teleportText teleport_text;
} VoxelWorld;


#endif
