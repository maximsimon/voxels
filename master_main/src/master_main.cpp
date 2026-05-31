// MASTER MAIN LAUNCHABLE - LAUNCHES SIMULATION IN A 1st THREAD AND LAUNCHES 2nd THREAD THAT IS USED TO COMMUNICATE WITH ANY MODULE THAT USES VOXEL_WORLD SIMULATION (e.g. ROS, RL setup)

#include "raylib.h"

#include "master_voxel.hpp"
#include "player_movement.hpp"
#include "ros_voxels.hpp"
#include "teleop_keys.hpp"
#include "master_main.hpp"
#include "config_master.hpp"

#include <opencv2/opencv.hpp>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <thread>

Observation master_observation;
Action master_action;
std::mutex mutex_observation;
std::mutex mutex_action;

// Pending teleport request, written by ROS callbacks and consumed once per simulation frame.  `pending` toggles to false once consumed so a single publish results in a single teleport.
struct TeleportRequest {
	float x;
	float z;
	float yaw_rad;
	bool pending;
};
static TeleportRequest master_teleport = { 0.0f, 0.0f, 0.0f, false };
static std::mutex mutex_teleport;

void master_ros_teleport(float x, float z, float yaw_rad) {
	std::lock_guard<std::mutex> lock(mutex_teleport);
	master_teleport.x = x;
	master_teleport.z = z;
	master_teleport.yaw_rad = yaw_rad;
	master_teleport.pending = true;
}

bool master_consume_teleport(float *x, float *z, float *yaw_rad) {
	std::lock_guard<std::mutex> lock(mutex_teleport);
	if (!master_teleport.pending) return false;
	*x = master_teleport.x;
	*z = master_teleport.z;
	*yaw_rad = master_teleport.yaw_rad;
	master_teleport.pending = false;
	return true;
}

// entire Voxel World is initiated and ran from this function, which is called by master_main.cpp main() function in its own neat little thread (hopefully)
void master_step_sim() {
	
	VoxelWorld* vw_instance;
	Observation* vw_observation = new Observation();
	Action* vw_action = new Action();// = (Action*)malloc(sizeof(Action));
	
	//Image mazemap_image = LoadImage(MAP_IMAGE_PATH);	//TODO: add some error handling and printing if file does not load
	Vector3 player_pose = PLAYER_POSE_INIT;
	Vector3 player_direction = PLAYER_DIRECTION_INIT; 
	int step_size = 2;		// in milliseconds
	
	vw_instance = init_sim(player_pose, player_direction, step_size);	// init voxel world simulation
	
	while (!WindowShouldClose() && !IsKeyPressed(KEY_Q)) { //observation_ = step_sim(vw_main_, action_);	
		mutex_action.lock();
		
		vw_action->linear_vel.x = master_action.linear_vel.x;
		vw_action->linear_vel.y = master_action.linear_vel.y;
		vw_action->linear_vel.z = master_action.linear_vel.z;
		
		vw_action->angular_vel.x = master_action.angular_vel.x;
		vw_action->angular_vel.y = master_action.angular_vel.y;
		vw_action->angular_vel.z = master_action.angular_vel.z;
		
		mutex_action.unlock();

		// Apply any pending external teleport request before stepping the sim, so the new pose is reflected in this frame's observation.
		float tp_x, tp_z, tp_yaw;
		if (master_consume_teleport(&tp_x, &tp_z, &tp_yaw)) {
			teleportWithYaw(&vw_instance->player_camera, tp_x, tp_z, tp_yaw);
		}

		// SIMULATION STEP
		step_sim(vw_instance, vw_action, vw_observation);	// SIMULATION STEP - this is where all calculations of what happens in simulation (i.e. in core_voxels) happens
		
		mutex_observation.lock();
		
		vw_observation->camera_front.copyTo(master_observation.camera_front);

		master_observation.position.x = vw_observation->position.x;
		master_observation.position.y = vw_observation->position.y;
		master_observation.position.z = vw_observation->position.z;
		
		master_observation.orientation.w = vw_observation->orientation.w;
		master_observation.orientation.x = vw_observation->orientation.x;
		master_observation.orientation.y = vw_observation->orientation.y;
		master_observation.orientation.z = vw_observation->orientation.z;
	
		master_observation.linear_vel.x = vw_observation->linear_vel.x;
		master_observation.linear_vel.y = vw_observation->linear_vel.y;
		master_observation.linear_vel.z = vw_observation->linear_vel.z;
		
		master_observation.angular_vel.x = vw_observation->angular_vel.x;
		master_observation.angular_vel.y = vw_observation->angular_vel.y;
		master_observation.angular_vel.z = vw_observation->angular_vel.z;	
	
		memcpy(master_observation.lidar_scan, vw_observation->lidar_scan, sizeof(master_observation.lidar_scan));
		
		mutex_observation.unlock();
	}
	
	CloseWindow();	//TODO: cleaner and move closing of simulation and cleanup to core_voxels + end the program, not just window (in future make simulation restartable from window or code as well)
}


// functioon with Axtion as input and Observation as output - ros_voxels (ROS2 loop) calls this function to interact with the simulation 
void master_ros_bridge(Action *ros_action, Observation *ros_observation) {
	
	mutex_action.lock();

	master_action.linear_vel.x = ros_action->linear_vel.x;
	master_action.linear_vel.y = ros_action->linear_vel.y;
	master_action.linear_vel.z = ros_action->linear_vel.z;
	
	master_action.angular_vel.x = ros_action->angular_vel.x;
	master_action.angular_vel.y = ros_action->angular_vel.y;
	master_action.angular_vel.z = ros_action->angular_vel.z;
	
	mutex_action.unlock();
	
	mutex_observation.lock();

	master_observation.camera_front.copyTo(ros_observation->camera_front);
	ros_observation->position.x = master_observation.position.x;
	ros_observation->position.y = master_observation.position.y;
	ros_observation->position.z = master_observation.position.z;
	
	ros_observation->orientation.w = master_observation.orientation.w;
	ros_observation->orientation.x = master_observation.orientation.x;
	ros_observation->orientation.y = master_observation.orientation.y;
	ros_observation->orientation.z = master_observation.orientation.z;

	ros_observation->linear_vel.x = master_observation.linear_vel.x;
	ros_observation->linear_vel.y = master_observation.linear_vel.y;
	ros_observation->linear_vel.z = master_observation.linear_vel.z;
	
	ros_observation->angular_vel.x = master_observation.angular_vel.x;
	ros_observation->angular_vel.y = master_observation.angular_vel.y;
	ros_observation->angular_vel.z = master_observation.angular_vel.z;
		
	memcpy(ros_observation->lidar_scan, master_observation.lidar_scan, sizeof(ros_observation->lidar_scan));
		
	mutex_observation.unlock();

}

int main(int argc, char * argv[]) {

	printf("MASTER MAIN STARTED \n");
	
	std::thread vw_step_thread(master_step_sim);		// start voxel world in thread
	std::thread ros_thread(master_ros);		// start ros in thread
	vw_step_thread.join();		// close voxel world thread
	ros_thread.join();		// close ros thread
	
	printf("MASTER MAIN ENDED \n");

}
