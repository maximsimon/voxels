// MASTER MAIN LAUNCHABLE - LAUNCHES SIMULATION IN A 1st THREAD AND LAUNCHES 2nd THREAD THAT IS USED TO COMMUNICATE WITH ANY MODULE THAT USES VOXEL_WORLD SIMULATION (e.g. ROS, RL setup)

#include "raylib.h"
#include "master_voxel.hpp"
#include "ros_voxels.hpp"
#include "master_main.hpp"
#include <opencv2/opencv.hpp>
#include <stdio.h>
#include <stdlib.h>
#include <thread>
#include <string>
#include <ament_index_cpp/get_package_share_directory.hpp>

Observation master_observation;
Action master_action;
std::mutex mutex_observation;
std::mutex mutex_action;

// entire Voxel World is initiated and ran from this function, which is called by master_main.cpp main() function in its own neat little thread (hopefully)
void master_step_sim() {
	
	VoxelWorld* vw_instance;
	Observation* vw_observation = new Observation();
	Action* vw_action = new Action();// = (Action*)malloc(sizeof(Action));
	
	// Get package share directory and construct path to map image
	std::string map_path;
	try {
		map_path = ament_index_cpp::get_package_share_directory("ros_voxels") + "/resources/map_images/mazemap_64.png";
	} catch (const std::exception& e) {
		// Fallback to original path for development
		map_path = "src/ros_voxels/core_voxels/resources/map_images/mazemap_64.png";
	}
	
	printf("Loading map from: %s\n", map_path.c_str());
	
	//TODO temp params here to get it running -> then move to yaml file
	Image mazemap_image = LoadImage(map_path.c_str());	//TODO: add some error handling and printing if file does not load
	
	// Check if image loaded successfully
	if (mazemap_image.data == NULL) {
		printf("ERROR: Failed to load map image from: %s\n", map_path.c_str());
		printf("Trying fallback path...\n");
		map_path = "src/ros_voxels/core_voxels/resources/map_images/mazemap_64.png";
		mazemap_image = LoadImage(map_path.c_str());
		if (mazemap_image.data == NULL) {
			printf("ERROR: Failed to load map image from fallback path: %s\n", map_path.c_str());
			return;
		}
	}
	
	Vector3 player_pose = { -2.0f, 0.5f, -2.0f };
	Vector3 player_direction = { -2.0f, 0.5f, -1.0f };
	int step_size = 2;		// in milliseconds
	
	vw_instance = init_sim(mazemap_image, player_pose, player_direction, step_size);	// init voxel world simulation
	
	while (!WindowShouldClose() && !IsKeyPressed(KEY_Q)) {
		mutex_action.lock();
		
		vw_action->linear_vel.x = master_action.linear_vel.x;
		vw_action->linear_vel.y = master_action.linear_vel.y;
		vw_action->linear_vel.z = master_action.linear_vel.z;
		
		vw_action->angular_vel.x = master_action.angular_vel.x;
		vw_action->angular_vel.y = master_action.angular_vel.y;
		vw_action->angular_vel.z = master_action.angular_vel.z;
		
		mutex_action.unlock();
		
	
		step_sim(vw_instance, vw_action, vw_observation);	// SIMULATION STEP
		
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
		
		mutex_observation.unlock();
	}
	
	CloseWindow();
}


//funkce co ma action v argumentu a vraci observation (ros vola tuhle funkci)
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
