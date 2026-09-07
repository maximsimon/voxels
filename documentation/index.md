# Voxel World — Documentation Index

This Index is supposed to provide a list of functions and variables used throughout Voxels code.
Index emphasizes completeness across the codebase rather clarity/readability.

## Functions

### master_main

#### master_main/include/master_main.hpp
`void master_step_sim()` - Run the simulation loop in its own thread (init, step until window closed). \
`void master_ros_bridge(Action *ros_action, Observation *ros_observation)` - Bridge function: ROS calls this to exchange action/observation with sim thread. \
`void master_ros_teleport(float x, float z, float yaw_rad)` - Queue a teleport request from ROS callback. Coordinates in raylib floor-plane axes. \
`bool master_consume_teleport(float *x, float *z, float *yaw_rad)` - Drain a pending teleport request in the sim thread. Returns true if consumed. \


### ros_voxels

#### include/ros_voxels.hpp
`void master_ros()` - Top-level ROS entry point. Initializes rclcpp, creates MasterRosNode + TeleopKeysNode, spins MultiThreadedExecutor. \
`class MasterRosNode : public rclcpp::Node` - Sends data inbetween the simulation and ROS2 topics. \

#### include/teleop_keys.hpp
`TeleopKeysNode::TeleopKeysNode()` - Opens keyboard input device, starts loop() in a background thread. \
`TeleopKeysNode::~TeleopKeysNode()` - Signals thread to stop, joins it, closes device fd. \
`void TeleopKeysNode::loop()` - Tansfers key presses to controls of player. Background thread: reads /dev/input/ events, toggles player_mode (P/O), publishes cmd_vel when player_mode is on. \
`std::string first_glob_match(const char* pattern)` - support for reading input device \
`std::string scan_proc_for_keyboard()` - support for reading input device \
`std::string resolve_keyboard_device()` - support for reading input device \

#### include/ros_axis_convert.hpp
`geometry_msgs::msg::Point rl_pos_to_ros(const Vector3& p)` - Raylib position to ROS REP-103 Point (swaps y/z). \
`geometry_msgs::msg::Vector3 rl_vec_to_ros(const Vector3& v)` - Raylib vector to ROS REP-103 Vector3 (swaps y/z). \
`geometry_msgs::msg::Quaternion rl_quat_to_ros(const Quaternion& q)` - Raylib quaternion to ROS quaternion (extracts 2D yaw around Z). \
`Vector3 ros_pos_to_rl(double x, double y, double z)` - ROS position to raylib Vector3 (swaps y/z). \
`Vector3 ros_vec_to_rl(const geometry_msgs::msg::Vector3& v)` - ROS vector to raylib Vector3 (swaps y/z). \
`Quaternion ros_quat_to_rl(double w, double x, double y, double z)` - ROS quaternion to raylib quaternion (2D yaw). \


### core_voxels

#### core_voxels/include/collisions.hpp
`bool CheckCollision(VoxelWorld *vw, Camera *camera, const float speed, Vector3 direction, Mesh *mesh)` - Check if player is colliding with any voxel. Uses ray-mesh intersection from boundary points. \
`void getVoxelBoundryPoints(Vector3 *boundry_points, Vector3 pose)` - Calculate position of 40 boundary points (bottom face sampling) of a voxel. \

#### core_voxels/include/small_handy_stuff.hpp
`Vector3 getUpDirection()` - Returns upward direction (0,1,0). \
`Vector3 getDownDirection()` - Returns downward direction (0,-1,0). \
`Vector3 getRightDirection(Camera camera)` - Rightward direction in camera's XZ plane. \
`Vector3 getLeftDirection(Camera camera)` - Leftward direction in camera's XZ plane. \
`Vector3 getBackDirection(Camera camera)` - Backward direction in camera's XZ plane. \
`Vector3 getForwardDirection(Camera camera)` - Forward direction in camera's XZ plane. \
`float getPlayerAngle(Camera camera)` - Heading angle of the camera in XZ plane, measured from +X axis, radians. \
`float getPlayerAngleDeg(Camera camera)` - Heading angle of the camera in XZ plane, measured from +X axis, degrees. \
`void printMap(mainMap main_map)` - Helper debugging function for printing map contents. \
`void drawLidarRays(VoxelWorld *vw, Observation *observation, bool in_player_view)` - Visualize lidar rays on screen as red lines. \

#### core_voxels/include/lidar.hpp
`static int cellOccupied(const mainMap &main_map, int grid_x, int grid_z)` - Look up the cell value at absolute grid coordinate (grid_x, grid_z); out-of-bounds treated as empty. \
`static float castRay(mainMap &main_map, Vector3 origin, float angle_rad, float max_range)` - Amanatides & Woo 2D DDA ray cast through the maze_map grid; returns hit distance (capped at max_range). \
`void updateLidar(VoxelWorld *vw, Observation *observation)` - Called by master_voxel: casts NUM_LIDAR_RAYS rays and fills observation->lidar_scan[]. \

#### core_voxels/include/map.hpp
`mainMap fetchMainMap(Image mazemap_img)` - Create map from image (white = empty, black = voxel). \
`int fetchCurrChunkIdx(mainMap main_map, Vector3 point)` - Fetch index of the chunk containing the given world point. \
`chunkMap fetchChunkMap(Color *pixels, int ch_x, int ch_z, int height_px)` - Map one chunk from pixel data. \
`HUE_TYPE getPixelHue(Color pixel)` - Get hue classification of a pixel. \
`void buildChunkMesh(mainMap *map, Mesh &mesh, int chunk_x, int chunk_z, int chunk_coord)` - Build mesh of one 16x16 chunk. \
`void buildVoxelWorldMesh(mainMap *map, Mesh *mesh)` - Build the full voxel world mesh from the map. \
`void genObject(Mesh &mesh, float x, float z, int *voxel_count, int hue_type)` - Generate a single voxel object at grid (x,z) and append to mesh. \
`void fetchCurrMapCoord(mainMap &main_map, Vector3 point)` - Fetch chunk index and cell index for a world point; writes to main_map.map_coords. \

#### core_voxels/include/textures.hpp
`void fetchTextureCoords(int texture_type, float *texcoords)` - Look up texture UV coordinates for a given texture type (tree/bush/building/brick). \

#### core_voxels/include/god_movement.hpp
`void CheckMovementEdit(Camera *camera)` - Check keyboard keys and move the edit (flying) camera. \
`void CameraMoveEdit(Camera *camera, Vector3 direction)` - Translate edit camera in given direction. \
`void CameraMoveUpDownEdit(Camera *camera, Vector3 direction)` - Translate edit camera up/down. \
`void CameraRotateEdit(Camera *camera, int HEADING)` - Rotate edit camera (RIGHT/LEFT/UP/DOWN). \
 
#### core_voxels/include/master_voxel.hpp
`VoxelWorld *init_sim(Image mazemap_image, Vector3 player_pose, Vector3 player_direction, int step_fq)` - Initialize the voxel world simulation: load map, create player, set up rendering. \
`void step_sim(VoxelWorld *vw, Action *action, Observation *observation)` - Advance simulation by one step: process action, update odometry/lidar, render POV and 3D view. \

#### core_voxels/include/support_for_master.hpp
`void movePlayerWithAction(VoxelWorld *vw, Action *action, Observation *observation)` - Apply action velocities to the player camera (forward/strafe/rotate), updates observation velocities. \
`void handleActionsAndKeys(VoxelWorld *vw, Action *action, Observation *observation)` - Handle keyboard controls and/or action-based movement. \
`bool checkControls(VoxelWorld *vw, Action *action, Observation *observation)` - Check keyboard input (P/O/V/T, teleport GUI, camera mode), returns true if player moved by keys. \

#### core_voxels/include/player_movement.hpp
`bool CheckMovement1person(VoxelWorld *vw, Camera *camera, Mesh *mesh, Observation *observation)` - Check keyboard keys that move the player (arrow keys, A/D rotation). \
`void CameraMove(Camera *camera, Vector3 direction, float speed)` - Translate camera in given direction at given speed. \
`void CameraRotate(Camera *camera, int HEADING, float turn_speed, Observation *observation)` - Rotate camera (RIGHT/LEFT/UP/DOWN). \
`float getPlayerAngle(Camera camera)` - Gets angle of player's voxel to the x axis, angle is in degrees. \
`void teleport(Camera *camera, Vector3 goal_pose)` - Teleport camera to goal_pose without changing orientation. \
`void teleportWithYaw(Camera *camera, float x, float z, float yaw_rad)` - Teleport floor-plane (x, z) and set heading; height forced to 0.5. \
`void teleportGUIinput(VoxelWorld *vw)` - Teleport camera based on in-simulation textbox input (press T, enter goal pose). \

#### core_voxels/include/odometry.hpp
`void updateOdometry(VoxelWorld *vw, Action *action, Observation *observation)` - Update observation's position, orientation (quaternion), and velocities from sim state. \
`Quaternion getQuaternionOrientation(VoxelWorld *vw)` - Compute current orientation quaternion from player camera direction. \

#### core_voxels/include/faces.hpp
`void fetchFront(int vc, float *v, float *n, Vector3 *vv, Vector3 *nv, int *vvc, int *vtc, unsigned short *indices)` - Generate front face of a voxel. \
`void fetchBack(...)` - Generate back face of a voxel. \
`void fetchTop(...)` - Generate top face of a voxel. \
`void fetchFloor(...)` - Generate floor face of a voxel. \
`void fetchRight(...)` - Generate right face of a voxel. \
`void fetchLeft(...)` - Generate left face of a voxel. \
`void MeshVoxel(Mesh& mesh, float pos_x, float pos_y, float pos_z, float angle, int *voxel_count, int texture_type, float width, float height, float length)` - Generate a complete voxel at given position and append to mesh. \


---

## Variables

### master_main

#### master_main/src/master_main.cpp — file globals
`Observation master_observation` - Shared observation between sim thread and ROS thread. \
`Action master_action` - Shared action between ROS thread and sim thread. \
`std::mutex mutex_observation` - Mutex for master_observation access. \
`std::mutex mutex_action` - Mutex for master_action access. \
`static TeleportRequest master_teleport` - Pending teleport request (x, z, yaw, pending flag). \
`static std::mutex mutex_teleport` - Mutex for teleport request. \

### ros_voxels

#### include/config.hpp
`bool MANUALLY_SET_DEVICE` - Set to 1 to use DEVICE path manually, 0 for auto-detection in teleop_keys.cpp. \
`std::string DEVICE` - Manual keyboard input device path (e.g. /dev/input/event4). \
`float ROS_SPEED` - Forward velocity in m/s when a movement key is held (default 3.0). \
`float ROS_TURN_SPEED` - Yaw rate in rad/s when a turn key is held (default 1.0). \

#### include/teleop_keys.hpp — class members
`rclcpp::Publisher<...>::SharedPtr pub_` - Publisher for /cmd_vel_subscriber (TwistStamped). \
`int device_fd_` - File descriptor for the opened keyboard input device. \
`std::thread thread_` - Background thread running the key-reading loop. \
`std::atomic<bool> running_` - Flag to signal the background thread to stop. \
`std::unordered_map<int, bool> keys_` - Current state of each key (evdev code to pressed/released). \
`bool player_mode` - Whether keyboard teleop publishing is active. \

### core_voxels

### core_voxels/include/data_types.hpp

#### constants
`#define NUM_LIDAR_RAYS 60` - Number of LiDAR rays per full 360 degree scan. \
`#define MAX_LIDAR_RANGE 10.0f` - Maximum LiDAR range in meters. \
`#define GRAY_VALUE(c)` - Grayscale luminance from RGB: (r+g+b)/3.0f. \
`extern const int screen_width` - Render window width in pixels (1600). \
`extern const int screen_height` - Render window height in pixels (850). \
`extern const int chunk_w` - Chunk width in cells (16). \
`extern const int chunk_h` - Chunk height in cells (16). \

#### VoxelWorld struct
`bool player_view` - Whether to show player POV or edit camera view. \
`bool player_mode` - Whether player (arrow keys) or edit camera (flying) is controlled. \
`Camera3D edit_camera` - Flying "god mode" camera. \
`Camera3D player_camera` - Player (robot) camera. \
`Camera3D current_camera` - Currently rendered camera (player or edit). \
`mainMap main_map` - The full chunked voxel world map. \
`Model player_model` - 1 red voxel, visual model of the player (robot). \
`float player_angle` - Player heading angle in degrees (for DrawModelEx). \
`RenderTexture2D camera_view_tex` - Off-screen render target for the POV camera. \
`Model *maze_model` - 3D model of the world (all voxel meshes). \
`Mesh *maze_mesh` - Raw mesh data for the world. \
`Model ground_model` - Visual model for the ground plane. \
`Model sky_model` - Visual model for the sky. \
`teleportText teleport_text` - Teleport text input box state (active when T pressed). \

#### Observation struct
`cv::Mat camera_front` - Last POV camera image (front camera of robot). \
`Vector3 position` - Current robot position in raylib world coords. \
`Quaternion orientation` - Current robot orientation. \
`Vector3 linear_vel` - Current linear velocity (m/s). \
`Vector3 angular_vel` - Current angular velocity (rad/s). \
`float lidar_scan[NUM_LIDAR_RAYS]` - LiDAR range readings for each ray (60 rays, full 360 degrees). \

#### Action struct
`Vector3 linear_vel` - Desired linear velocity (m/s). \
`Vector3 angular_vel` - Desired angular velocity (rad/s). \

#### chunkMap struct
`int map[16*16]` - 16x16 grid: 1 = voxel present, 0 = empty. \
`int voxel_count` - Number of non-empty voxels in this chunk. \
`Vector3 chunk_position` - World position of this chunk's origin. \

#### mainMap struct
`chunkMap *chunks` - Dynamically allocated array of all chunks. \
`int width_px` - Total world width in pixels (cells). \
`int height_px` - Total world height in pixels (cells). \
`int width_chunks` - Number of chunks along X axis. \
`int height_chunks` - Number of chunks along Z axis. \
`int chunk_side` - Size of one chunk side in cells (chunks are square). \
`mapCoords map_coords` - Convenience struct with current chunk index + cell index. \

#### mapCoords struct
`int chunk` - Index of a chunk. \
`int cell` - Index of a cell inside that chunk. \

#### teleportText struct
`bool text_active` - Whether the teleport textbox is open. \
`int letter_count` - Number of characters entered. \
`Rectangle text_box` - Screen-space rectangle for the textbox. \
`int MAX_INPUT_CHARS` - Maximum allowed input characters. \
`char text[]` - Flexible array member holding the text string. \

#### core_voxels/include/config_core.hpp
`const float SPEED = 0.05f` - Player movement speed per frame. \
`const float TURN_SPEED = 0.01f` - Player turn speed per frame in radians. \
`const float SPEED_GOD = 0.5f` - Edit/god camera movement speed per frame. \
`const float TURN_SPEED_GOD = 0.05f` - Edit/god camera turn speed per frame. \

#### core_voxels/include/textures.hpp
`const float tree[48]` - UV texture atlas coordinates for the "tree" (pink) voxel texture. \
`const float bush[48]` - UV texture atlas coordinates for the "bush" (green) voxel texture. \
`const float building[48]` - UV texture atlas coordinates for the "building" (blue) voxel texture. \
`const float brick[48]` - UV texture atlas coordinates for the "brick" (red) voxel texture. \

#### core_voxels/include/player_movement.hpp — enums
`enum HEADING { RIGHT, LEFT, UP, DOWN }` - Direction constants for camera rotation. \
`int SCREENSHOT_COUNTER` - Counter for screenshot filenames (incremented on SPACE key). \

#### core_voxels/include/map.hpp — enums
`enum HUE_TYPE { white, red, green, pink, blue, unknown }` - Color classification for map pixels (determines voxel texture type). \

