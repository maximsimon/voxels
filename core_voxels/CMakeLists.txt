cmake_minimum_required(VERSION 3.5)
project(core_voxels)

# Compile as C++14
set(CMAKE_CXX_STANDARD 14)

# Include headers
include_directories(
  include
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


