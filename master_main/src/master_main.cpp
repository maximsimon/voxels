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
	
	//TODO temp params here to get it running -> then move to yaml file
	Image mazemap_image = LoadImage("src/core_voxels/resources/map_images/mazemap_64.png");	//TODO: add some error handling and printing if file does not load
	Vector3 player_pose = { -2.0f, 0.5f, -2.0f };
	Vector3 player_direction = { -2.0f, 0.5f, -1.0f };
	int step_size = 2;		// in milliseconds
	
	vw_instance = init_sim(mazemap_image, player_pose, player_direction, step_size);	// init voxel world simulation
	
	while (!WindowShouldClose() && !IsKeyPressed(KEY_Q)) { //observation_ = step_sim(vw_main_, action_);	
		mutex_action.lock();
		*vw_action = master_action;
		mutex_action.unlock();
		//printf("\n\n action 0: %f \n\n", master_action.linear_vel.x);
		//vw_observation = step_sim(vw_instance, vw_action);
		step_sim(vw_instance, vw_action, vw_observation);
		
		mutex_observation.lock();
		//master_observation = vw_observation;
		vw_observation->camera_front.copyTo(master_observation.camera_front);
		mutex_observation.unlock();
	}
	
	CloseWindow();	//TODO: cleaner and move closing of simulation and cleanup to core_voxels + end the program, not just window (in future make simulation restartable from window or code as well)
}


//funkce co ma action v argumentu a vraci observation (ros vola tuhle funkci)
void master_ros_bridge(Action *ros_action, Observation *ros_observation) {
	//Observation *ros_observation;
	
	mutex_action.lock();
	master_action = *ros_action;
	mutex_action.unlock();
	
	//if (ros_action != NULL) printf("action is not NULL yaaay \n\n\n\n\n\n\n");

	mutex_observation.lock();
	//ros_observation = master_observation;
	master_observation.camera_front.copyTo(ros_observation->camera_front);
	mutex_observation.unlock();

	//return ros_observation;
}

int main(int argc, char * argv[]) {

	printf("MASTER MAIN STARTED \n");
	
	std::thread vw_step_thread(master_step_sim);		// start voxel world in thread
	std::thread ros_thread(master_ros);		// start ros in thread
	vw_step_thread.join();		// close voxel world thread
	ros_thread.join();		// close ros thread
	
	// NEXT STEP: then camera view return
	
	printf("MASTER MAIN ENDED \n");

}
