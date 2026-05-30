# Architecture

Find here outlined architecture of the code and the repository structure.

## 1. System Architecture

### 1.1 High-Level Structure

```
┌──────────────────────────────────────────────────────────────────┐
│                        master_main (executable)                    │
│                                                                    │
│  ┌──────────────────────┐         ┌─────────────────────────┐      │
│  │  Simulation Thread    │         │     ROS 2 Thread         │      │
│  │  (master_step_sim)    │         │     (master_ros)         │      │
│  │                       │         │                          │      │
│  │  ┌─────────────────┐  │         │  ┌──────────────────┐   │      │
│  │  │   core_voxels    │  │ mutex   │  │  MasterRosNode   │   │      │
│  │  │  (static lib)    │◄─┼─────────┼─►│  - publishes     │   │      │
│  │  │                  │  │ shared  │  │  - subscribes    │   │      │
│  │  │ - init_sim()     │  │ state   │  └──────────────────┘   │      │
│  │  │ - step_sim()     │  │         │  ┌──────────────────┐   │      │
│  │  │ - rendering      │  │         │  │  TeleopKeysNode  │   │      │
│  │  │ - collision      │  │         │  │  - keyboard →    │   │      │
│  │  │ - odometry       │  │         │  │    cmd_vel        │   │      │
│  │  └─────────────────┘  │         │  └──────────────────┘   │      │
│  └──────────────────────┘         └─────────────────────────┘      │
└──────────────────────────────────────────────────────────────────┘
```

### 1.2 Threading Model

Two threads run concurrently:

1. **Simulation Thread** — owns the raylib window, runs `step_sim()` at the configured frame rate (50 Hz by default). Handles keyboard input (during god mode mostly, player mode is handled `TeleopKeysNode`) via raylib's `IsKeyDown()`, renders the 3D scene, and computes observations.

2. **ROS Thread** — spins a `rclcpp::MultiThreadedExecutor` with two nodes:
   - `MasterRosNode` — bridge between ROS topics and the simulation
   - `TeleopKeysNode` — reads raw keyboard events from `/dev/input/*`

Data passes between threads through three global structs protected by `std::mutex`:

| Global | Mutex | Direction |
|--------|-------|-----------|
| `master_action` | `mutex_action` | ROS → Simulation |
| `master_observation` | `mutex_observation` | Simulation → ROS |
| `master_teleport` | `mutex_teleport` | ROS → Simulation |

---

## 2. Directory Structure

```
ros_voxels/
├── CMakeLists.txt                 # Top-level build (ROS 2 package)
├── package.xml                    # ROS 2 package manifest
├── LICENSE                        # Apache 2.0
├── README.md                      # Project README
├── docs/
│   └── (documentation files)
├── include/                       # Public headers for ros_voxels lib
│   ├── ros_voxels.hpp             #   master_ros() entry point
│   ├── ros_axis_convert.hpp       #   raylib ↔ ROS coordinate conversion
│   └── teleop_keys.hpp            #   TeleopKeysNode class
├── src/
│   ├── ros_voxels.cpp             #   MasterRosNode implementation
│   └── teleop_keys.cpp            #   TeleopKeysNode implementation
├── core_voxels/                   # Simulation engine (no ROS deps)
│   ├── CMakeLists.txt             #   Standalone CMake build
│   ├── include/                   #   Headers
│   │   ├── data_types.hpp         #     Core structs (VoxelWorld, Observation, Action)
│   │   ├── master_voxel.hpp       #     init_sim(), step_sim()
│   │   ├── map.hpp                #     Map generation from image
│   │   ├── faces.hpp              #     Voxel mesh generation
│   │   ├── textures.hpp           #     Texture atlas coordinates
│   │   ├── collisions.hpp         #     Collision detection
│   │   ├── player_movement.hpp    #     First-person camera controls
│   │   ├── god_movement.hpp       #     Free-fly camera controls
│   │   ├── odometry.hpp           #     Orientation quaternion
│   │   ├── small_handy_stuff.hpp  #     Direction helpers
│   │   └── support_for_master.hpp #     Action/key handler
│   ├── src/                       #   Implementations (see software_design.md)
│   └── resources/                 #   Assets
│       ├── map_images/            #     image map definitions
│       └── textures/              #     Texture atlases, sky, ground
└── master_main/                   # Main executable
    ├── CMakeLists.txt
    ├── include/
    │   └── master_main.hpp        #   Bridge function declarations
    └── src/
        └── master_main.cpp        #   main(), threads, bridge
```



