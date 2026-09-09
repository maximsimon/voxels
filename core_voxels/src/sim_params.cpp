// RUNTIME-TUNABLE SIMULATION PARAMETERS - see sim_params.hpp

#include "sim_params.hpp"

#include "raylib.h"

SimParams sim_params;

int simActiveLidarRays() {
	if (sim_params.lidar_rays < 1) return 1;
	if (sim_params.lidar_rays > NUM_LIDAR_RAYS) return NUM_LIDAR_RAYS;
	return sim_params.lidar_rays;
}

float simStepDelta() {
	if (sim_params.fixed_dt > 0.0f) return sim_params.fixed_dt;
	return GetFrameTime();
}
