cmake_minimum_required(VERSION 3.10)
project(core_voxels)

# Compile as C++14
set(CMAKE_CXX_STANDARD 17)

# Include headers
include_directories(
  include
  resources/worlds
  ${OpenCV_INCLUDE_DIRS}
)

find_package(OpenCV REQUIRED)

# Library
add_library(${PROJECT_NAME} STATIC
  src/master_voxel.cpp
  src/god_movement.cpp
  src/collisions.cpp
  src/faces.cpp
  src/map.cpp
  src/player_movement.cpp
  src/small_handy_stuff.cpp
  src/support_for_master.cpp
  src/odometry.cpp
  src/textures.cpp
  src/lidar.cpp
  src/world_loading.cpp
  src/world_config.cpp
)
# Link raylib (if core uses it)
target_link_libraries(core_voxels
    raylib
    pthread
    ${OpenCV_LIBS}  # link OpenCV    
)

# Install (optional)
install(TARGETS ${PROJECT_NAME}
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
        RUNTIME DESTINATION bin)
install(DIRECTORY include/ DESTINATION include)
install(DIRECTORY resources/worlds/ DESTINATION resources/worlds)


