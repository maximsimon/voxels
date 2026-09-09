// STANDALONE CLI - runs the Voxel World simulation without Python

#include "master_main.hpp"
#include <stdio.h>

int main() {
	printf("MASTER MAIN STARTED \n");
	master_init_sim();			// initialize the simulation once at launch
	while (master_step_sim()) {}		// step the simulation until the window/quit key requests exit
	master_end_sim();			// release the world and close the window
	printf("MASTER MAIN ENDED \n");
	return 0;
}
