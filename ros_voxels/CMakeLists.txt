cmake_minimum_required(VERSION 3.5)
project(ros_voxels)

# Default to C++14
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 14)
endif()

# Find dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)
find_package(sensor_msgs REQUIRED)

# Include your own library headers
include_directories(
  include
  ../core_voxels/include   # Path to sim_core headers
)

# Link your library
add_executable(${PROJECT_NAME}_main_node src/ros_voxels_main_node.cpp)

# Add dependency library
add_subdirectory(../core_voxels core_voxels_build)  # Build sim_core from workspace

target_link_libraries(${PROJECT_NAME}_main_node
  core_voxels            # Link the library
)

# Link ROS2
ament_target_dependencies(${PROJECT_NAME}_main_node
  rclcpp
  std_msgs
  sensor_msgs
)

install(TARGETS
  ${PROJECT_NAME}_main_node
  DESTINATION lib/${PROJECT_NAME}
)

ament_package()
