cmake_minimum_required(VERSION 3.5)
project(master_main)

# Default to C++14
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
endif()

# Find dependencies
find_package(ament_cmake REQUIRED)
# ROS2 packages
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)
find_package(OpenCV REQUIRED)

include_directories(
  include
  ../core_voxels/include   # Path to simulation (core_voxels) headers
  ../ros_voxels/include   
  ${OpenCV_INCLUDE_DIRS}
)

# Add executable
add_executable(master_main src/master_main.cpp)

add_subdirectory(
  ../core_voxels core_voxels_build
)
add_subdirectory(
  ../ros_voxels ros_voxels_build
)
# Library

# Link core_voxels library
target_link_libraries(master_main
  core_voxels
  ros_voxels
  ${OpenCV_LIBS}  # link OpenCV    
)

# Link ROS2 if available
ament_target_dependencies(master_main rclcpp std_msgs)

# Install executable (optional)
install(TARGETS master_main
    DESTINATION lib/${PROJECT_NAME}
)
