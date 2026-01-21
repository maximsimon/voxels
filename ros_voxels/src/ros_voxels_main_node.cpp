#include <chrono>
#include <memory>
#include <string>
#include <cstdlib>

#include "raylib.h"
#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"
#include "sensor_msgs/msg/image.hpp"

#include "master_voxel.h"

#include <GL/glew.h>
#include <GL/gl.h>          // OpenGL core functions
#include <GL/glext.h>       // OpenGL extensions (for PBO)

using namespace std::chrono_literals;

/* This example creates a subclass of Node and uses a fancy C++11 lambda
* function to shorten the callback syntax, at the expense of making the
* code somewhat more difficult to understand at first glance. */

class MinimalPublisher : public rclcpp::Node
{
public:
	MinimalPublisher()
	: Node("minimal_publisher")
	{
		publisher_ = this->create_publisher<sensor_msgs::msg::Image>("camera_front", 10);
		
		//TODO temp params here to get it running -> then move to yaml file
		Image mazemap_image = LoadImage("src/core_voxels/resources/map_images/mazemap_64.png");	//TODO: add some error handling and printing if file does not load
		Vector3 player_pose = { -2.0f, 0.5f, -2.0f };
		Vector3 player_direction = { -2.0f, 0.5f, -1.0f };
		int step_size = 2;		// in milliseconds
		vw_main_ = init_sim(mazemap_image, player_pose, player_direction, step_size);
	
		// Timer to call publish_every_spin() every 100ms
		timer_ = this->create_wall_timer(
			std::chrono::milliseconds(step_size),
			std::bind(&MinimalPublisher::publish_every_spin, this)
        	);
	}

private:
	rclcpp::TimerBase::SharedPtr timer_;
	rclcpp::Publisher<sensor_msgs::msg::Image>::SharedPtr publisher_;
		
	VoxelWorld* vw_main_;
	Observation* observation_;
	Action* action_;// = (Action*)malloc(sizeof(Action));

	void publish_every_spin() {
		
		if (!IsKeyPressed(KEY_Q)) observation_ = step_sim(vw_main_, action_);	
		else {
			CloseWindow();	//TODO: cleaner and move closing of simulation and cleanup to core_voxels
			rclcpp::shutdown();
		}

		// build and publish camera view from Voxel World simulation	
		auto image_msg = sensor_msgs::msg::Image();
		image_msg.height = screen_height;
		image_msg.width = screen_width;
		image_msg.encoding = "rgba8"; // matches GL_RGBA
		image_msg.is_bigendian = 0;
		image_msg.step = screen_width * 4;
		image_msg.data.assign(observation_->camera_img, observation_->camera_img + screen_width*screen_height*4); // fill data from GPU
		
		publisher_->publish(image_msg);
	}
};

int main(int argc, char * argv[])
{
	rclcpp::init(argc, argv);
	rclcpp::spin(std::make_shared<MinimalPublisher>());
	rclcpp::shutdown();
	return 0;
}
