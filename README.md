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

For ROS2 testbed (e.g. for __bearnavs__) switch to `ros_api` branch.


### Controls
`arrow keys` for movement (translation)

`w s a d` for rotation

`p` toggles player mode - if on move with player voxel, if off move with god view (flying)

`v` toggles view mode - if on see from player perspective, if off see from god perspective (but not possible to move in god mode but view from player perspective)

`pg_up pg_down` fly up and down if you are in god mode (player mode off)

`space bar` takes screenshot

`t` opens input box for goal position where robot (player) will be teleported after confirming by `enter`.
Input ints, floats, negative numbers, whatever...
Sepearate numbers by spaces; 1st number is x, 2nd y, 3rd z, y coord will be internally overwriten to 0.5 so that robot does not move up or down and stays in 2D space.
If you enter nothing and press `enter` robot teleports to (0, 0.5 0).
__Warning:__ if you teleport the robot into a wall, you won't be able to move it by arrow keys and will need to teleport out of it.

## ROS API
This branch is Voxel World connected with ROS2.
It is not working yet.

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
colcon build
source install/setup.bash
```
Run:
```
./install/master_main/lib/master_main/master_main
```
Now a window with the simulation (see bottom of this README) should display and the simulation should be running.

*Note: If it throws `Segmentation error` and the simulation window crashes, it is most likely structural error in how you built it.*

My file strcture where it's running looks like this:
```
voxel_world_ws/src/
├── core_voxels
│   ├── CMakeLists.txt
│   ├── include
│   ├── resources
│   └── src
├── documentation
│   ├── DOCUMENTATION.md
│   └── figures
├── README.md
└── ros_voxels
    ├── CMakeLists.txt
    ├── include
    ├── LICENSE
    ├── package.xml
    └── src
```
and I run `colcon build` and `ros2 run` from `voxel_world_ws/`.

If you want to run this with __bearnav__ then:
Compile and run normally (with `colcon build` and `ros2 run`).
Now simulation is running independently and ROS2 is active -> you can launch bearnav in completiely different workspace and treat the topics and action services provided by this simulation as you would if they were coming from robot (Note: this statement is the goal, however it's not impemented yet).

See `documentation/` for more detailed description of code structure, available functions etc.

![Voxel World - God mode view](documentation/figures/god_camera_view.png)
![Voxel World - Player mode view](documentation/figures/player_camera_view.png)
