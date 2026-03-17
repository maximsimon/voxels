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
find_package(geometry_msgs REQUIRED)
find_package(nav_msgs REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(tf2_ros REQUIRED)
find_package(OpenCV REQUIRED)
find_package(cv_bridge REQUIRED)

# Include your own library headers
include_directories(
  include
  ../core_voxels/include
  ../master_main/include
  ${OpenCV_INCLUDE_DIRS}
)

# Library
add_library(${PROJECT_NAME} STATIC
  src/ros_voxels.cpp
)

# Link ROS2
ament_target_dependencies(${PROJECT_NAME}
  rclcpp
  std_msgs
  sensor_msgs
  cv_bridge
  geometry_msgs
  nav_msgs
  tf2_ros
)

install(TARGETS
  ${PROJECT_NAME}
  DESTINATION lib/${PROJECT_NAME}
)

target_link_libraries(${PROJECT_NAME}
  ${OpenCV_LIBS}  # link OpenCV    
)

# Export include dirs if needed
#target_include_directories(ros_voxels PUBLIC
#    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
#    $<INSTALL_INTERFACE:include>
#)

ament_package()
