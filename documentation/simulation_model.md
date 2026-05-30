# Simulation Model

This file contains description of simulation's properties, how it is structured, how it behaves: building map, voxel grid, movements, sensors, coords, limitations etc.

## 1. World Model

The simulated world is a **voxel grid** procedurally generated from a 2D image (png, jpg, ...). Each pixel in the image maps to a position in the 3D world, and the pixel's color determines whether a voxel is placed and what type it is.

<table>

God mode view of world generated from 16x16 pixel map (the red cube with an arrow is the robot)
<img src="documentation/figures/god_view_aligned.png" width="800">

<td align="center">
Map (16 x 16 px) from which the world was generated
<img src="documentation/figures/map_sharp.png" width="270">
</td>

<td align="center">
Atlas of visual textures pasted on voxels based on pixel colors
<img src="documentation/figures/atlas.png" width="270">
</td>

</table>

*Source images for textures are courtesy of: wikimedia: tree, grass (for ground); unsplash: bricks; pngtree: bush; pinimg: building*
### 1.1 Voxel Encoding

Adjustable in `core_voxels/src/textures.cpp`. `core_voxels/src/map.cpp` contains definition of color's hue range and  `core_voxels/include/map.hpp` contains color strings enumeration.  We plan to move these definitions to `config.hpp` or something.

| Pixel Color        | Hue Range | Voxel Type     | Size (w×h×d)  | Stack Height |
| ------------------ | --------- | -------------- | ------------- | ------------ |
| White (gray > 150) | —         | Air (no voxel) | —             | —            |
| Red                | < 70°     | Wall           | 1 × 2 × 1     | 1            |
| Green              | 70–160°   | Bush           | 1 × 1 × 1     | 1            |
| Blue               | 160–290°  | Building       | 1 × 2 × 1     | 5            |
| Pink               | > 290°    | Tree           | 0.5 × 1 × 0.5 | 4            |

### 1.2 World Geometry

- **Grid spacing**: 1 unit per voxel (in raylib coordinate space)
- **Vertical**: Y-up, ground at y = 0
- **Player/robot height**: Camera pinned to y = 0.5 (eye height of ~0.5 m above ground)
- **Map size**: Arbitrary, multiples of 16 pixels (chunk size). Tested maps range from 64×64 to 512×512 pixels.
- **Chunking**: World is divided into 16×16 voxel chunks for mesh management
- **Rendering:** Currently all chunks are rendered all the time causing large maps (e.g. 2000x2000) to slow down the simulation.

### 1.3 Chunking System

The world is organized into `chunkMap` structures to keep mesh sizes manageable:

```
chunkMap {
    int map[16 × 16];        // 256 ints: hue type per cell
    int voxel_count;          // number of voxels in this chunk (for mesh indexing)
    Vector3 chunk_position;   // world position of first voxel in chunk
}

mainMap {
    chunkMap *chunks;         // array of chunks (width_chunks × height_chunks)
    int width_px, height_px;  // world dimensions in pixels
    int width_chunks, height_chunks;
    int chunk_side;           // always 16
}
```


## 2. Kinematics Model

The simulation is NOT physical.
### 2.1 Kinematics

The robot is modeled as a **2D holonomic agent** with **instantaneous velocity control**:

Translation movement of the robot is handled roughly as:

	direction = Normalize(direction)
    direction = direction * speed
	
	camera->position = camera->position + direction
	camera->target = camera->target + direction

Rotation is handled by a raylib function.


## 3. Sensor Models

### 3.1 Monocular Camera

The camera sensor is rendered by drawing the 3D scene from the player camera's perspective into an offscreen `RenderTexture2D` (raylib's render-to-texture mechanism).


**Camera parameters**:

| Parameter | Value |
|-----------|-------|
| Resolution | 1600 × 850 pixels (window size) |
| FOV (vertical) | 45° |
| Projection | Perspective |
| Moves in a plane | `player_camera.position.y = 0.5` |
| Orientation | Player heading direction |
We have never tried to change camera parameters.

### 3.2 Odometry

Odometry is computed directly from the simulation state — it is **ground truth** with no added noise.

| Field            | Source                                  | Units                 |
| ---------------- | --------------------------------------- | --------------------- |
| Position         | Direct copy of `player_camera.position` | meters (raylib units) |
| Orientation      | Extracted from camera view matrix       | quaternion (REP-103)  |
| Linear velocity  | Velocity applied this frame             | m/s                   |
| Angular velocity | Yaw rate applied this frame             | rad/s                 |

**Reference frames**:
- Odometry frame (`frame_id`): `"odom"`
- Child frame (`child_frame_id`): `"base_link"`

### 3.3 LiDAR

A ray-casting 2D LiDAR that simulates a horizontal laser scanner. It is **ground truth** with no added noise or drop-outs.

| Parameter          | Value                                | Parameter name  |
| ------------------ | ------------------------------------ | --------------- |
| Number of rays     | 60 (full 360°)                       | NUM_LIDAR_RAYS  |
| Max range          | 10.0 m                               | MAX_LIDAR_RANGE |
| Angular resolution | 6° (360° / 60)                       |                 |
| First ray heading  | Aligned with player camera direction |                 |
	Parameters are in `core_voxels/include/config_core.hpp`

Each simulation step, `updateLidar()` in `lidar.cpp` casts `NUM_LIDAR_RAYS` rays in a full circle around the player's current position. Each ray uses the **Amanatides & Woo DDA (Digital Differential Analyzer)** algorithm to traverse the `mainMap` grid cell-by-cell, checking `cellOccupied()` at each step. The distance to the first occupied cell is stored in `observation->lidar_scan[]`, capped at `MAX_LIDAR_RANGE` (10.0 m).

**Important note:** The simulated LiDAR currently detects hits only from the map and thus ignores width of voxels. Each cell is either occupied or free.

**Grid traversal** (`castRay()`):
- Converts the world-space ray origin to a grid cell index by truncating the position coordinates.
- Steps through adjacent cells using DDA, computing `tMax` (distance to next grid boundary) on each axis.
- At each visited cell, calls `cellOccupied()` which maps the grid coordinate to the correct chunk and local cell via integer division and modulo.
- Returns the `t` distance at the first occupied cell, or `MAX_LIDAR_RANGE` if no hit.

The LiDAR operates in 2D on the XZ plane — it ignores height (Y) entirely, treating each grid cell as a full-height obstacle column regardless of its actual voxel height.

## 4. Coordinate Systems

### 4.1 Raylib Coordinate System (Simulation Internal)

```
  y (up)
  ▲
  │
  │   z (lateral)
  │  ╱
  │ ╱
  ────────► x (forward)
```

All simulation calculations (positions, velocities, rendering) use raylib's Y-up convention.

### 4.2 ROS Coordinate System (REP-103)

```
  z (up)
  ▲
  │
  │   y (left)
  │  ╱
  │ ╱
  ────────► x (forward)
```

### 4.3 Mapping Between Systems

```
ROS x (forward) = raylib x
ROS y (left)    = raylib z
ROS z (up)      = raylib y
```

The conversion is implemented in `ros_axis_convert.hpp` and used at every ROS↔raylib boundary.

### 4.4 Orientation Handling

For the 2D yaw-only simulator, the relationship simplifies to:

```
ROS yaw = raylib yaw
```



## 5. Environmental Assumptions

1. **Gravity**: Not simulated. Height is pinned at y = 0.5.
2. **Lighting**: Not simulated. Raylib renders with flat shading.
3. **Friction**: Not simulated.
4. **Air resistance**: Not simulated.
5. **Terrain**: Flat ground plane at y = 0. No elevation changes.
6. **Weather**: Clear sky. Static star-field skybox.
7. **Dynamic objects**: None. The world is static.
8. **Multi-agent**: Not supported. Single robot only.
9. **Noise**: None. All sensors return ground-truth values.
10. **Time**: Real-time simulation with wall-clock frame timing.

**Important timing note**: The simulation and ROS threads run independently. The simulation thread blocks on `step_sim()` which includes rendering and image readback — this can take longer than 20 ms on slower hardware, causing the simulation to run slower than real-time. The ROS thread continues publishing at 50 Hz regardless, using the last available observation.
