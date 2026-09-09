cmake_minimum_required(VERSION 3.10)
project(ros_voxels)

# Default to C++14
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

# Find dependencies
find_package(OpenCV REQUIRED)
find_package(X11 REQUIRED)
find_package(Python3 COMPONENTS Interpreter Development REQUIRED)

# Find pybind11 (pip install pybind11)
execute_process(
  COMMAND ${Python3_EXECUTABLE} -m pybind11 --cmakedir
  OUTPUT_VARIABLE pybind11_DIR
  OUTPUT_STRIP_TRAILING_WHITESPACE
  ERROR_QUIET)
find_package(pybind11 CONFIG PATHS ${pybind11_DIR} NO_DEFAULT_PATH REQUIRED)

# Find numpy headers for the pybind11 module
execute_process(
  COMMAND ${Python3_EXECUTABLE} -c "import numpy; print(numpy.get_include())"
  OUTPUT_VARIABLE NumPy_INCLUDE_DIR
  OUTPUT_STRIP_TRAILING_WHITESPACE
  ERROR_QUIET)

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
  core_voxels/src/sim_params.cpp
)

target_link_libraries(core_voxels
  ${RAYLIB_LIBRARY}
  ${X11_LIBRARIES}
  m
  pthread
  ${OpenCV_LIBS}
)

set_target_properties(core_voxels PROPERTIES POSITION_INDEPENDENT_CODE ON)

# Master main library (static) - sim glue: init/step/teleport + public action/observation buffers
add_library(master_main_core STATIC
  master_main/src/master_main.cpp
)

target_link_libraries(master_main_core
  core_voxels
  ${RAYLIB_LIBRARY}
  ${X11_LIBRARIES}
  m
  pthread
  ${OpenCV_LIBS}
)

set_target_properties(master_main_core PROPERTIES POSITION_INDEPENDENT_CODE ON)

# Python bridge module - exposes Init()/Step()/Teleport()/Close() to Python
pybind11_add_module(voxel_sim
  src/main.cpp
)

target_include_directories(voxel_sim PRIVATE ${NumPy_INCLUDE_DIR})

target_link_libraries(voxel_sim PRIVATE
  master_main_core
  core_voxels
  ${RAYLIB_LIBRARY}
  ${X11_LIBRARIES}
  m
  pthread
  ${OpenCV_LIBS}
)

# Standalone CLI executable - runs the simulation without Python
add_executable(master_main
  src/cli_main.cpp
)

target_link_libraries(master_main
  master_main_core
)

# Install targets
install(TARGETS
  core_voxels
  master_main_core
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