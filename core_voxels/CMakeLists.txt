cmake_minimum_required(VERSION 3.5)
project(core_voxels)

# Compile as C++14
set(CMAKE_C_STANDARD 11)

# Include headers
include_directories(include)

# Library
add_library(${PROJECT_NAME} STATIC
  src/master_voxel.c
  src/camera_movement.c
  src/collisions.c
  src/faces.c
  src/map.c
  src/player_movement.c
  src/small_handy_stuff.c
)
# Link raylib (if core uses it)
target_link_libraries(core_voxels
    raylib
    m
    pthread
)

# Install (optional)
install(TARGETS ${PROJECT_NAME}
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
        RUNTIME DESTINATION bin)
install(DIRECTORY include/ DESTINATION include)


