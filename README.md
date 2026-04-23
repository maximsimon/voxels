# voxels
Rendering a voxel world using raylib C library.
Voxel is a 3D pixel - i.e. a cube.
World is generated from black voxels.
Location of each World voxel is defined by black and white pixels in .png image.
1 Player voxel (red) is spawned.
Player can be controlled by arrows.
Additionaly "God" view (flight and no collisions) is availabe.
Keys are used to switch between "Player" mode and "God mode".

We are in the proccess of making this both ROS2 testbed and python API environment for reinforcement learning (RL).

For ROS2 testbed (e.g. for __bearnavs__) switch to `ros2_api` branch.

For ROS1 testbed (e.g. also for __bearnavs__) switch to `ros1_api` branch.

### Controls
#### God mode movement
`arrow keys` for movement (translation)

`w s a d` for rotation

`pg_up pg_down` fly up and down if you are in god mode (player mode off)


#### Player mode movement
`w s a d` for movement (translation)

`k l` for rotation


`p` toggles player mode on - if on move with player voxel, if off move with god view (flying)

`o` toggles player mode off

`v` toggles view mode - if on see from player perspective, if off see from god perspective (but not possible to move in god mode but view from player perspective)

`space bar` takes screenshot

`t` opens input box for goal position where robot (player) will be teleported after confirming by `enter`.
Input ints, floats, negative numbers, whatever...
Sepearate numbers by spaces; 1st number is x, 2nd y, 3rd z, y coord will be internally overwriten to 0.5 so that robot does not move up or down and stays in 2D space.
If you enter nothing and press `enter` robot teleports to (0, 0.5 0).

__Warning:__ if you teleport the robot into a wall, you won't be able to move it by arrow keys and will need to teleport out of it.

## ROS API
`ros1_api` and `ros2_api` branches are Voxel World connected with ROS1 or ROS2 respectively.
They act as a robotic testbed.
Currently robot can be controlled with arrow keys or ROS action commands and the only implemented sensors are front facing monocular camera and odometry.

### QUICK START

__You need to install raylib [(https://www.raylib.com/)]__
If raylib is up and running, then create ROS2 workspace, clone this branch.
```
mkdir -p voxel_world_ws
cd voxel_world_ws
git clone --branch ros_api git@github.com:maximsimon/voxels.git
```
Now it's sketchy (I really have to fix this later) - reaname this git (*voxels*) to `src/` and then compile and source:
```
mv voxels src
```

The following differs for ROS1 (`ros1_api`) and ROS2 (`ros2_api`).

#### ROS2
```
colcon build
source install/setup.bash
```
Run:
```
./install/master_main/lib/master_main/master_main
```
Now a window with the simulation (see bottom of this README) should display and the simulation should be running.

#### ROS1
```
catkin build 
cd build
cmake ../src/master_main/
make
cd ..
catkin build
source devel/setup.bash
./build/master_main
```
Now a window with the simulation (see bottom of this README) should display and the simulation should be running.

*Note: 1st `catkin build` creates `build`, `devel` and `logs` folders and compiles `ros_voxels`, you can use `cd build` instead though, just so you have it to run `cmake` from.
`cmake ../src/master_main` prepares build for the non-ros part (`core_voxels` and `master_main`), `make` then compiles the non-ros part.
Finally `catkin build` builds the ros part (`ros_voxels`) and `./build/master_main` launches the entire simulation including ROS*

---

*Note: If it throws `Segmentation error` and the simulation window crashes, it is most likely structural error in how you built it or the location from when you are running is invalid with the path set for loading map_image (file `master_main/src/master_main.cpp`).*

My file strcture where it's running looks like this:
```
voxel_world_ws/src/
├── core_voxels
│   ├── CMakeLists.txt
│   ├── include
│   ├── resources
│   └── src
├── documentation
│   ├── DOCUMENTATION.md
│   └── figures
├── README.md
└── ros_voxels
    ├── CMakeLists.txt
    ├── include
    ├── LICENSE
    ├── package.xml
    └── src
```
and I compile and run from `voxel_world_ws` or `voxel_world_ws/build` for `make` for `ros1_api` testbed. 

If you want to run this with __bearnav__ then:
Compile and run normally (with `roscore` running somewher for `ros1_api`).
Now simulation is running independently and ROS is active -> you can launch bearnav in completiely different workspace and treat the topics and action services provided by this simulation as you would if they were coming from robot. 

See `documentation/` for more detailed description of code structure, available functions etc.

![Voxel World - God mode view](documentation/figures/god_camera_view.png)
![Voxel World - Player mode view](documentation/figures/player_camera_view.png)
