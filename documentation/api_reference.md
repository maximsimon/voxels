# API Reference

Here we provide more detailed information about important aspects of the code (functions, varaibles, publishers etc).

## 1. Node: master_ros_node

The main ROS 2 node that bridges the simulation with the ROS 2 ecosystem.

### 1.1 Published Topics

| Topic | Type | Rate | QoS | Description |
|-------|------|------|-----|-------------|
| `/camera_front_publisher` | `sensor_msgs/Image` | 50 Hz | Best-effort, KeepLast(5) | First-person RGB camera view (BGR8 encoding) |
| `/camera_front_publisher/camera_info` | `sensor_msgs/CameraInfo` | 50 Hz | Best-effort, KeepLast(5) | Camera intrinsics (lazy-initialized) |
| `/odometry_publisher` | `nav_msgs/Odometry` | 50 Hz | Best-effort, KeepLast(5) | Ground-truth pose and twist |
| `/cmd_vel_publisher` | `geometry_msgs/TwistStamped` | 50 Hz | Best-effort, KeepLast(5) | Current commanded velocity (echoed back) |
| `/lidar_scan` | `sensor_msgs/LaserScan` | 50 Hz | Best-effort, KeepLast(5) | LiDAR on the robot - range and number of rays controlled in core_voxels |
### 1.2 Subscribed Topics

| Topic | Type  | QoS | Description |
|-------|------|-----|-------------|
| `cmd_vel_subscriber` | `geometry_msgs/TwistStamped` | Best-effort, KeepLast(10) | Velocity commands for the robot |
| `/initialpose` | `geometry_msgs/PoseWithCovarianceStamped` | Reliable, KeepLast(1) | Teleport robot to pose (rviz 2D Pose Estimate compatible) |

### 1.3 TF Transforms

| Frame | Type | Rate | Description |
|-------|------|------|-------------|
| `odom` → `base_link` | Static (updated) | 50 Hz | Robot pose in world frame |

**Frame convention**:
- `odom`: World-fixed frame, origin at world coordinate origin (raylib `(0, 0, 0)`)
- `base_link`: Robot body frame, origin at `player_camera.position` (y = 0.5)
- `camera_front_publish`: Frame ID for camera image header (note: typo "publish" per codebase convention)


## 2. Node: teleop_keys

Standalone keyboard teleoperation node. Reads raw Linux input events and publishes velocity commands.

| Topic | Type | Rate | QoS | Description |
|-------|------|------|-----|-------------|
| `/cmd_vel_subscriber` | `geometry_msgs/TwistStamped` | 50 Hz (while keys held) | Best-effort, KeepLast(10) | Keyboard-derived velocity commands |

| Parameter | Value | Description |
|-----------|-------|-------------|
| `ROS_SPEED` | 3.0 m/s | Forward/strafe velocity |
| `ROS_TURN_SPEED` | 1.0 rad/s | Yaw rate |
| `TELEOP_KEYS_DEVICE` | string path | Override keyboard input device path (e.g., `/dev/input/event4`) |

### 2.4 Key Mapping

| Key | Action | Twist Component 
|-----|--------|----------------
| W | Move forward | `twist.linear.x` |
| S | Move backward | `twist.linear.x` |
| A | Strafe left | `twist.linear.y` |
| D | Strafe right | `twist.linear.y` |
| K | Rotate left | `twist.angular.z` |
| L | Rotate right | `twist.angular.z` |
| P | Enable player mode | — |
| O | Disable player mode | — |

---

## 3. Simulation Entry Point: master_main

Handles interaction of ROS with the simulation (`ros_voxels` with `core_voxels`).

`int main(int argc, char * argv[])` ... executed main when simulation launched
calls `master_step_sim()`

`void master_step_sim()` runs the simulation: by calling `sim_step()` in a loop - `step_sim() is in `core_voxels`
Handles mutexes - passes `Action` to simulation: `step_sim(vw_instance, vw_action, vw_observation);`, and saves the resulting `Observation`.

`void master_ros_bridge(Action *ros_action, Observation *ros_observation)` is called by the ROS node (`ros_voxels`) - transfers mutexes, so that ROS can pass Action in and recieve the current Observation

