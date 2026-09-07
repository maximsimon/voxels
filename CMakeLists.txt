cmake_minimum_required(VERSION 3.10)
project(ros_voxels)

# Default to C++14
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
endif()

# Find dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(nav_msgs REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(tf2_ros REQUIRED)
find_package(OpenCV REQUIRED)
find_package(cv_bridge REQUIRED)
find_package(ament_index_cpp REQUIRED)
find_package(X11 REQUIRED)

# Find raylib
find_library(RAYLIB_LIBRARY raylib REQUIRED)
find_path(RAYLIB_INCLUDE_DIR raylib.h REQUIRED)

# Include directories
include_directories(
  include
  core_voxels/include
  core_voxels/resources/worlds
  master_main/include
  ${OpenCV_INCLUDE_DIRS}
  ${RAYLIB_INCLUDE_DIR}
)

# Core voxels library (static)
add_library(core_voxels STATIC
  core_voxels/src/master_voxel.cpp
  core_voxels/src/god_movement.cpp
  core_voxels/src/collisions.cpp
  core_voxels/src/faces.cpp
  core_voxels/src/textures.cpp
  core_voxels/src/map.cpp
  core_voxels/src/player_movement.cpp
  core_voxels/src/small_handy_stuff.cpp
  core_voxels/src/support_for_master.cpp
  core_voxels/src/odometry.cpp
  core_voxels/src/lidar.cpp
  core_voxels/src/world_loading.cpp
  core_voxels/src/world_config.cpp
)

target_link_libraries(core_voxels
  ${RAYLIB_LIBRARY}
  ${X11_LIBRARIES}
  m
  pthread
  ${OpenCV_LIBS}
)

# ROS voxels library
add_library(ros_voxels STATIC
  src/ros_voxels.cpp
  src/teleop_keys.cpp
)

ament_target_dependencies(ros_voxels
  rclcpp
  std_msgs
  sensor_msgs
  cv_bridge
  geometry_msgs
  nav_msgs
  tf2_ros
)

target_link_libraries(ros_voxels
  ${OpenCV_LIBS}
)

# Master main executable
add_executable(master_main
  master_main/src/master_main.cpp
)

target_link_libraries(master_main
  core_voxels
  ros_voxels
  ${RAYLIB_LIBRARY}
  ${X11_LIBRARIES}
  m
  pthread
  ${OpenCV_LIBS}
)

ament_target_dependencies(master_main
  rclcpp
  std_msgs
  geometry_msgs
  nav_msgs
  sensor_msgs
  tf2_ros
  cv_bridge
  ament_index_cpp
)

# Install targets
install(TARGETS
  core_voxels
  ros_voxels
  DESTINATION lib/${PROJECT_NAME}
)

install(TARGETS
  master_main
  DESTINATION lib/${PROJECT_NAME}
)

# Install headers
install(DIRECTORY include/
  DESTINATION include/${PROJECT_NAME}
)

install(DIRECTORY core_voxels/include/
  DESTINATION include/${PROJECT_NAME}
)

install(DIRECTORY master_main/include/
  DESTINATION include/${PROJECT_NAME}
)

# Install resources
install(DIRECTORY core_voxels/resources/
  DESTINATION share/${PROJECT_NAME}/resources
)

ament_package()
