# Software Design

In here we try to give an idea behind how the code works - the logic, connections etc.

## 1. Library: `core_voxels` (Static Library)

A self-contained simulation engine with zero ROS dependencies. It handles voxel world generation, 3D rendering via raylib, collision detection, player/camera controls, simulating sensors etc.

### 1.1 Data Model

#### `VoxelWorld` (`data_types.hpp`)

The central simulation state struct. All simulation components mutate or read this struct.\
Allocated via `malloc` in `init_sim()` (`master_voxel.cpp:65`)

```cpp
typedef struct VoxelWorld {
    bool player_view;           // true = first-person POV; false = god/third-person
    bool player_mode;           // true = keyboard controls robot; false = god camera
    Camera3D edit_camera;       // god-mode free-fly camera
    Camera3D player_camera;     // robot-mounted first-person camera
    Camera3D current_camera;    // whichever camera is rendered on-screen
    mainMap main_map;           // chunked voxel occupancy grid
    Model player_model;         // red voxel mesh for the robot
    float player_angle;         // heading in degrees for model rotation
    RenderTexture2D camera_view_tex; // render target for first-person camera
    Model *maze_model;          // array of chunk models (one per chunk)
    Mesh *maze_mesh;            // array of chunk meshes
    Model ground_model;         // textured ground plane
    Model sky_model;            // textured hemisphere sky
    teleportText teleport_text; // in-sim teleport input box state
} VoxelWorld;
```
 	

#### `Observation` (`data_types.hpp`)

The sensor reading produced each simulation step.

| Field                        | Type         | Source                                                  |
| ---------------------------- | ------------ | ------------------------------------------------------- |
| `camera_front`               | `cv::Mat`    | Render-to-texture → raylib Image → OpenCV BGR           |
| `position`                   | `Vector3`    | Direct copy of `player_camera.position`                 |
| `orientation`                | `Quaternion` | Computed from camera view matrix                        |
| `linear_vel`                 | `Vector3`    | Accumulated from action/keys this frame                 |
| `angular_vel`                | `Vector3`    | Accumulated from action/keys this frame                 |
| `lidar_scan[NUM_LIDAR_RAYS]` | `float`      | Laser Scan gathered by raytracing through map (mainMap) |

#### `Action` (`data_types.hpp)

The control input consumed by the robot each frame. Info gathered from ROS2 topics.

| Field | Type | Meaning |
|-------|------|---------|
| `linear_vel` | `Vector3` | Linear velocity in raylib coords (x=forward, z=lateral) |
| `angular_vel` | `Vector3` | Angular velocity (y=yaw) |

#### `mainMap` (`data_types.hpp`)

Map of the full voxel world, composed of an array of 16x16 chunks with dimension metadata. Each element in mainMap is a 16x16 chunk and each element in a chunk is an integer encoding type of voxel column spawned at corresponding voxel world position.

| Field           | Type        | Description                                                          |
| --------------- | ----------- | -------------------------------------------------------------------- |
| `chunks`        | `chunkMap*` | Dynamically allocated array of chunks (width_chunks × height_chunks) |
| `width_px`      | `int`       | Total world width in pixels (cells)                                  |
| `height_px`     | `int`       | Total world height in pixels (cells)                                 |
| `width_chunks`  | `int`       | Number of chunks along X axis                                        |
| `height_chunks` | `int`       | Number of chunks along Z axis                                        |
| `chunk_side`    | `int`       | Size of one chunk side in cells (always 16)                          |
| `map_coords`    | `mapCoords` | Current chunk index and cell index for the player position           |

#### `chunkMap` (`data_types.hpp`)

One 16×16 tile of the voxel world. Each cell encodes the voxel type at that position.

| Field            | Type      | Description                                                                                                                                                 |
| ---------------- | --------- | ----------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `map[16*16]`     | `int`     | Flat 16×16 grid: 0 = air (no voxel), nonzero = hue type from map image (e.g. green pixel can encode 2 voxels high column covered in 'grass' visual texture) |
| `voxel_count`    | `int`     | Number of occupied voxels in this chunk (for mesh indexing) (We recommend to not touch this - it is mainly used in `MeshVoxel()` in `faces.cpp`)            |
| `chunk_position` | `Vector3` | World position of this chunk's origin                                                                                                                       |

#### `mapCoords` (`data_types.hpp`)

Convenience struct for working with the maze map.

| Field | Type | Description |
|-------|------|-------------|
| `chunk` | `int` | Index of a chunk |
| `cell` | `int` | Index of a cell inside that chunk |

### 1.2 Lifecycle

`master_voxel.cpp` contains 3 functions through which the simulation is controlled by `master_main`. They are the only ones called from outside of `core_voxels`:
```
init_sim()          → Allocates VoxelWorld struct, loads textures, builds mesh
                        ↓
while (running) {   → Simulation loop
    step_sim()        → Handles input (`Action`), moves player, renders simulation, fill `Observation`
}
                        ↓
end_sim()           → CloseWindow() (does not free memory)
```

### 1.3 Code Flow

The simulation is driven by two concurrent threads started from the executable `master_main/src/master_main.cpp`:

1. **Simulation thread** — runs `master_step_sim()`: loads the map image, calls `init_sim()` to build the world, then loops calling `step_sim()` each frame.
2. **ROS thread** — runs `master_ros()`: spins ROS nodes that exchange data with the sim thread via mutex-protected globals (`master_action`, `master_observation`, `master_teleport`, `ros_action`, `ros_observation`).

**Per-frame logic** inside `step_sim()` (`core_voxels/src/master_voxel.cpp`) executes in this order:

1. **Movement** — `handleActionsAndKeys()` in `support_for_master.cpp` first checks keyboard input via `checkControls()` in `player_movement.cpp`. If no keys are pressed, it falls back to `movePlayerWithAction()` which applies the external `Action` velocities. Every movement call gates through `CheckCollision()` in `collisions.cpp`, which probes 40 boundary points around the future position using ray-mesh intersection against the chunk meshes. Actual camera translation/rotation happens via `CameraMove()` / `CameraRotate()` in `player_movement.cpp`.

2. **Odometry** — `updateOdometry()` in `odometry.cpp` snapshots the camera position and computes an orientation quaternion from the view matrix.

3. **LiDAR** — `updateLidar()` in `lidar.cpp` casts configurable number of rays in a full 360° circle around the player using the Amanatides & Woo DDA grid traversal algorithm over the `mainMap` grid, storing hit distances in `observation->lidar_scan[]`.

4. **Rendering** — The first-person POV is always rendered to an offscreen texture and converted to a `cv::Mat` to simulate robot front-facing camera in `observation->camera_front`. The main 3D view (either player or edit camera) draws the chunk meshes (voxels), the robot, sky dome, ground plane, and optional LiDAR ray visualization.

**Map pipeline** (`map.cpp`): A 2D pixel image (png, jpg, ...) is parsed into a `mainMap` of 16×16 pixel chunks via `fetchMainMap()`. `getPixelHue()` classifies each pixel into a voxel type (white = air (no voxel), red/green/blue/pink = wall/bush/building/tree (but can be remaped in `textures.cpp`)). `buildVoxelWorldMesh()` iterates chunks and calls `buildChunkMesh()` → `genObject()` → `MeshVoxel()` (in `faces.cpp`) to produce raylib mesh geometry for rendering and collision.

**Cross-thread communication**: The ROS thread writes an `Action` and reads an `Observation` through `master_ros_bridge()` in `master_main.cpp`. Teleport requests from ROS (`/initialpose` topic) go through `master_ros_teleport()` / `master_consume_teleport()` — a lock-free single-slot queue consumed once per sim frame.

## 2. Library: `ros_voxels` (Static Library)

ROS 2 communication layer. Through it the tested software interacts with the simulation. Contains two nodes: `MasterRosNode` and `TeleopKeysNode`.

### 2.1 `MasterRosNode` (`src/ros_voxels.cpp)

**Constructor** :
- Creates publishers and subscribers
- Creates a 20ms wall timer for periodic publishing

**`publish_every_spin()`**:
Called at 50 Hz via wall timer. Performs:
1. Calls `master_ros_bridge()` to exchange data with simulation thread
2. Builds all ROS messages from the observation (shared timestamp)
3. Lazily initializes camera intrinsics on first frame
4. Publishes all messages back-to-back

**`action_callback()`**:
Receives velocity commands. Converts ROS REP-103 axes to raylib axes via `ros_vec_to_rl()`, then calls `master_ros_bridge()`.

**`teleport_callback()`**:
Receives `/initialpose` (from rviz "2D Pose Estimate"). Extracts yaw from quaternion, then calls `master_ros_teleport()`.

**Time-out logic** :
If no `cmd_vel` received for >0.2 seconds, the action is zeroed (safety timeout).

### 2.2 `TeleopKeysNode` (`src/teleop_keys.cpp`)

Reads raw keyboard events from a Linux input device (`/dev/input/event*`) and publishes `TwistStamped` to `/cmd_vel_subscriber`.

**`loop()`**:
Separate thread that runs at 50 Hz. Reads all pending `input_event` structs via non-blocking `read()`, updates a key-state map, and publishes `TwistStamped` with velocities mapped from held keys (WSAD = movement, K/L = rotation).

**Key mapping**:
| Key | Effect | ROS Velocity |
|-----|--------|-------------|
| W | Forward | linear.x = +3.0 m/s |
| S | Backward | linear.x = −3.0 m/s |
| A | Strafe left | linear.y = +3.0 m/s |
| D | Strafe right | linear.y = −3.0 m/s |
| K | Rotate left | angular.z = +1.0 rad/s |
| L | Rotate right | angular.z = −1.0 rad/s |
| P | Enable player mode | (toggle) |
| O | Disable player mode | (toggle) |

---
