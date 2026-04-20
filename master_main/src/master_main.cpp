// MASTER MAIN LAUNCHABLE - LAUNCHES SIMULATION IN A 1st THREAD AND LAUNCHES 2nd THREAD THAT IS USED TO COMMUNICATE WITH ANY MODULE THAT USES VOXEL_WORLD SIMULATION (e.g. ROS, RL setup)

#include "raylib.h"
#include "master_voxel.hpp"
#include "ros_voxels.hpp"
#include "master_main.hpp"
#include <opencv2/opencv.hpp>
#include <stdio.h>
#include <stdlib.h>
#include <thread>

Observation master_observation;
Action master_action;
std::mutex mutex_observation;
std::mutex mutex_action;

// entire Voxel World is initiated and ran from this function, which is called by master_main.cpp main() function in its own neat little thread (hopefully)
void master_step_sim() {
	
	VoxelWorld* vw_instance;
	Observation* vw_observation = new Observation();
	Action* vw_action = new Action();// = (Action*)malloc(sizeof(Action));
	
	//TODO: add some error handling and printing if file does not load
	//TODO temp params here to get it running -> then move to yaml file
	Image mazemap_image = LoadImage("src/core_voxels/resources/map_images/map_paper.png");	//TODO: add some error handling and printing if file does not load
	Vector3 player_pose = { -2.0f, 0.5f, -2.0f };
	Vector3 player_direction = { -2.0f, 0.5f, -1.0f };
	int step_size = 2;		// in milliseconds
	
	vw_instance = init_sim(mazemap_image, player_pose, player_direction, step_size);	// init voxel world simulation
	
	while (!WindowShouldClose() && !IsKeyPressed(KEY_Q)) { //observation_ = step_sim(vw_main_, action_);	
		mutex_action.lock();
		
		vw_action->linear_vel.x = master_action.linear_vel.x;
		vw_action->linear_vel.y = master_action.linear_vel.y;
		vw_action->linear_vel.z = master_action.linear_vel.z;
		
		vw_action->angular_vel.x = master_action.angular_vel.x;
		vw_action->angular_vel.y = master_action.angular_vel.y;
		vw_action->angular_vel.z = master_action.angular_vel.z;
		
		mutex_action.unlock();
		
	
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
		
		mutex_observation.unlock();
	}
	
	CloseWindow();	//TODO: cleaner and move closing of simulation and cleanup to core_voxels + end the program, not just window (in future make simulation restartable from window or code as well)
}


//funkce co ma action v argumentu a vraci observation (ros vola tuhle funkci)
void master_ros_bridge(Action *ros_action, Observation *ros_observation) {
	//Observation *ros_observation;
	
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
