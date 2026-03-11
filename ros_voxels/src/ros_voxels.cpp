#include <chrono>
#include <memory>
#include <string>
#include <cstdlib>

#include "raylib.h"
#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"
#include "std_msgs/msg/int32.hpp"
#include "sensor_msgs/msg/image.hpp"

#include <opencv2/opencv.hpp>
#include <cv_bridge/cv_bridge.hpp>

#include "ros_voxels.hpp"
#include "data_types.hpp"
#include "master_main.hpp"

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
		publisher_ = this->create_publisher<sensor_msgs::msg::Image>("image_publisher", 10);
	
		int step_size = 2;
		// Timer to call publish_every_spin() every 100ms
		timer_ = this->create_wall_timer(
			std::chrono::milliseconds(step_size),
			std::bind(&MinimalPublisher::publish_every_spin, this)
        	);

	}

private:
	rclcpp::TimerBase::SharedPtr timer_;
	rclcpp::Publisher<sensor_msgs::msg::Image>::SharedPtr publisher_;
	sensor_msgs::msg::Image::SharedPtr image_msg;
	Observation* observation_;
	Action* action_;

	void publish_every_spin() {
		observation_ = master_ros_bridge(action_);
		if (observation_ != NULL) {
			image_msg = cv_bridge::CvImage(std_msgs::msg::Header(), "bgr8", observation_->camera_front).toImageMsg();
			publisher_->publish(*image_msg);
		}
	}
};

void master_ros() {
	rclcpp::init(0, nullptr);
	//TODO: do the executorSafe nodes thign
	rclcpp::spin(std::make_shared<MinimalPublisher>());
	rclcpp::shutdown();
}
