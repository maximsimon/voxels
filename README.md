# Ros Voxels README

A lightweight, voxel-based robot simulator for **ROS 2** with [raylib](https://www.raylib.com/) rendering.

Generate a block world from a 2D pixel image, drive a robot around it with keyboard or ROS commands, and get camera images + odometry + lidar scan as real-time ROS topics. Designed as a testbed for high-level navigation strategies.

Extended documentation can be found in `documentation/`.

[***Paper*** (or rather extended abstract)](https://mobile-robotics-hub.github.io/workshop2026/papers/LoWi2026_P11.pdf)

![Voxel World - God mode view](documentation/figures/god_view.png)
![Voxel World - God mode view](documentation/figures/player_view.png)

---

## Quick Start

```bash
## Prerequisites: 

### ROS 2 Jazzy, raylib, OpenCV
sudo apt install libraylib-dev libopencv-dev

#### add user to the input group for reading key presses to control robot
sudo usermod -a -G input $USER
now log out and back in, when you run `groups` you should see `input` listed

###raylib
either from __[(https://www.raylib.com/)]__ or by:
sudo add-apt-repository ppa:texus/raylib
sudo apt update
sudo apt install libraylib5-dev

## Build
mkdir -p ~/voxel_world_ws/src
cd ~/voxel_world_ws/src
git clone <this-repo> ros_voxels
cd ~/voxel_world_ws
colcon build
source install/setup.bash

# Run (must be from workspace root for resource paths)
./install/ros_voxels/lib/ros_voxels/master_main
```

A window opens showing Voxels - the voxel world simulation. Fly with **Arrows**, **PgUp, PgDn** and **WASD**. Press **P** to enable player mode (**O** to disble it), then use **WASD** to move and **KL** to turn. Press **V** for first-person view (robot POV).

---

## Features

- **Procedural voxel world** generated from a 2D pixel image (.png, .jpg, ...)
- **Monocular camera** — `sensor_msgs/Image` at 50 Hz (BGR8, 1600×850)
- **Ground-truth odometry** — `nav_msgs/Odometry` + TF (`odom` → `base_link`)
- **planar LiDAR laser scan** — `sensor_msgs/LaserScan`
- **Velocity control** — `cmd_vel_subscriber` (`TwistStamped`)
- **Teleport** — `/initialpose` for resetting robot position
- **Keyboard teleop** — raw `/dev/input/` reads for low-latency control
- **1st and 3rd person view** — god mode (free-fly) + player mode (first-person)

---

## ROS 2 Interface

| Topic | Type | Direction |
|-------|------|-----------|
| `/camera_front_publisher` | `sensor_msgs/Image` | Published |
| `/odometry_publisher` | `nav_msgs/Odometry` | Published |
| `/lidar_scan` | `sensor_msgs/LaserScan` | Published |
| `/cmd_vel_publisher` | `geometry_msgs/TwistStamped` | Published |
| `/cmd_vel_subscriber` | `geometry_msgs/TwistStamped` | Subscribed |
| `/initialpose` | `geometry_msgs/PoseWithCovarianceStamped` | Subscribed |

**TF**: `odom` → `base_link` (broadcast at 50 Hz)

---

## Controls

| Key | Mode | Action |
|-----|------|--------|
| P | Any | Enable player mode |
| O | Any | Disable player mode |
| V | Any | Toggle first-person view |
| WASD | Player | Move robot |
| KL | Player | Rotate robot |
| Arrows | God | Move camera |
| WASD | God | Rotate camera |
| PgUp/PgDn | God | Move up/down |
| T | Any | Teleport input ("x z yaw_deg") |
| Space | Any | Screenshot |
| Q or Esc | Any | Quit |

---

## Repository Structure

```
ros_voxels/
├── CMakeLists.txt              # ROS 2 package build
├── package.xml                 # ROS 2 manifest
├── core_voxels/                # Simulation engine (no ROS deps)
│   ├── include/                #   Headers
│   ├── src/                    #   Implementation (10 source files)
│   └── resources/              #   Map images + textures
├── ros_voxels/                 # ROS 2 glue library
│   ├── include/
│   └── src/
├── master_main/                # Main entry point (executable)
│   ├── include/
│   └── src/
└── documentation/                       # Full documentation suite
    ├── architecture.md
    ├── software_design.md
    ├── simulation_model.md
    ├── api_reference.md
    └── README.md (this file)
```

---

## Documentation
Find detailed documentation in `documentation/`:

| Document              | Content                                                        |
| --------------------- | -------------------------------------------------------------- |
| `README.md`           | Intro to documentation                                         |
| `index.md`            | List of all functions and some variables - with short comments |
| `architecture.md`     | System architecture, data flow, threading model                |
| `software_design.md`  | Detailed module design, algorithms, data types                 |
| `simulation_model.md` | World model, physics, sensor models, coordinate frames         |
| `api_reference.md`    | Full ROS 2 interface, library API reference                    |

---

## Requirements

- **ROS 2** Humble / Iron / Jazzy (tested on Jazzy)
- **C++14** compiler
- **raylib** (tested on ≥ 4.5)
- **OpenCV** (tested on ≥ 4.2)
- **X11**, OpenGL (for raylib rendering)

---

## License

Apache 2.0. See `LICENSE`.
