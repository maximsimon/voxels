#pragma once
#include <cmath>
#include "raylib.h"
#include "geometry_msgs/msg/point.hpp"
#include "geometry_msgs/msg/vector3.hpp"
#include "geometry_msgs/msg/quaternion.hpp"

// Single source of truth for the raylib (Y-up) <-> ROS REP-103 (Z-up)
// translation used at every boundary in ros_voxels.cpp / teleop_keys.cpp.
//
//   ROS x (forward) <-> raylib x   (forward floor coord)
//   ROS y (left)    <-> raylib z   (lateral floor coord)
//   ROS z (up)      <-> raylib y   (height)
//
// For the 2D yaw-only simulator, ROS yaw == raylib yaw (no sign flip), so
// the y<->z component swap is exact for orientation quaternions as well.
// If the sim ever models 3D motion (roll/pitch), revisit rl_quat_to_ros:
// the simple swap is no longer exact for general rotations.

namespace ros_voxels {

// --- Outbound: raylib -> ROS --------------------------------------------

inline geometry_msgs::msg::Point rl_pos_to_ros(const Vector3& p) {
    geometry_msgs::msg::Point r;
    r.x = p.x; r.y = p.z; r.z = p.y;
    return r;
}

// TF translations and twist linear/angular share this Vector3 layout.
inline geometry_msgs::msg::Vector3 rl_vec_to_ros(const Vector3& v) {
    geometry_msgs::msg::Vector3 r;
    r.x = v.x; r.y = v.z; r.z = v.y;
    return r;
}

inline geometry_msgs::msg::Quaternion rl_quat_to_ros(const Quaternion& q) {
    // Raylib stores the camera orientation as a quaternion that rotates the
    // local camera-forward axis (raylib -Z) into the world heading direction
    // — there is a built-in 90° offset between "camera neutral" and "robot
    // facing +X". A naive component swap doesn't account for that offset.
    //
    // For the 2D yaw-only simulator we recover the heading by rotating the
    // raylib local-forward (0,0,-1) by q, take atan2 in the floor plane (this
    // matches getPlayerAngle's atan2(z, x) convention, and the codebase's
    // "raylib yaw == ROS yaw" relation), then re-encode as a +Z-axis
    // ROS quaternion.
    const double dir_x = -2.0 * (q.x * q.z + q.y * q.w);
    const double dir_z = -(1.0 - 2.0 * (q.x * q.x + q.y * q.y));
    const double yaw = std::atan2(dir_z, dir_x);
    geometry_msgs::msg::Quaternion r;
    r.w = std::cos(yaw / 2.0);
    r.x = 0.0;
    r.y = 0.0;
    r.z = std::sin(yaw / 2.0);
    return r;
}

// --- Inbound: ROS -> raylib ---------------------------------------------

inline Vector3 ros_pos_to_rl(double x, double y, double z) {
    return Vector3{ static_cast<float>(x),
                    static_cast<float>(z),
                    static_cast<float>(y) };
}

inline Vector3 ros_vec_to_rl(const geometry_msgs::msg::Vector3& v) {
    return Vector3{ static_cast<float>(v.x),
                    static_cast<float>(v.z),
                    static_cast<float>(v.y) };
}

inline Quaternion ros_quat_to_rl(double w, double x, double y, double z) {
    // Inverse of rl_quat_to_ros: take ROS yaw, build a raylib camera
    // quaternion that rotates local -Z into the same world heading.
    // (Currently unused — teleport_callback extracts yaw scalar directly —
    // but kept for symmetry so the helper is round-trippable.)
    const double yaw  = std::atan2(2.0 * (w * z + x * y),
                                   1.0 - 2.0 * (y * y + z * z));
    const double beta = std::atan2(-std::cos(yaw), -std::sin(yaw));
    return Quaternion{ 0.0f,
                       static_cast<float>(std::sin(beta / 2.0)),
                       0.0f,
                       static_cast<float>(std::cos(beta / 2.0)) };
}

}  // namespace ros_voxels
