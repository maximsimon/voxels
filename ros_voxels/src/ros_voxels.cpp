#include <chrono>
#include <memory>
#include <string>
#include <cstdlib>

#include "raylib.h"
#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"
#include "std_msgs/msg/int32.hpp"
#include "sensor_msgs/msg/image.hpp"
#include "geometry_msgs/msg/twist.hpp"

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

class ObsActNode : public rclcpp::Node
{
public:
	ObsActNode()
	: Node("obs_act_node")
	{
		obs_publisher_ = this->create_publisher<sensor_msgs::msg::Image>("camera_front_publisher", 10);		// publisher that fetces observation from voxel world simulation and publishes it to topic, for outside programs to see inside the simulation trough ROS
		act_subscriber_ = this->create_subscription<geometry_msgs::msg::Twist>(
		    "action_subscriber",
		    10,
		    std::bind(&ObsActNode::action_callback, this, std::placeholders::_1)
		);	// subscriber that gets action from outside and passes it to voxel world simulation, for outside programs to control an agent in the simulation trough ROS
	
		int step_size = 2;
		// Timer to call publish_every_spin() every 100ms
		timer_ = this->create_wall_timer(
			std::chrono::milliseconds(100),
			std::bind(&ObsActNode::publish_every_spin, this)
        	);

	}

private:
	rclcpp::TimerBase::SharedPtr timer_;
	rclcpp::Publisher<sensor_msgs::msg::Image>::SharedPtr obs_publisher_;
	rclcpp::Subscription<geometry_msgs::msg::Twist>::SharedPtr act_subscriber_;
	sensor_msgs::msg::Image::SharedPtr image_msg;
	Observation* observation_ = new Observation();
	Action* action_ = new Action();

	void action_callback(const geometry_msgs::msg::Twist::SharedPtr action_msg) {
		//NEXT_STEP: why doesnt this work? it throws segmentation fault
		action_->linear_vel = {(float)action_msg->linear.x, (float)action_msg->linear.y, (float)action_msg->linear.z};
		action_->angular_vel = {(float)action_msg->angular.x, (float)action_msg->angular.y, (float)action_msg->angular.z};
	
		////TODO: figure out how to streamline fetching obseervation (every callback? every spin? both? figure it out)
	
		if (action_ != NULL) {
			master_ros_bridge(action_, observation_);
		}
	}

	void publish_every_spin() {
		master_ros_bridge(action_, observation_);
		if (observation_ != NULL) {
			image_msg = cv_bridge::CvImage(std_msgs::msg::Header(), "bgr8", observation_->camera_front).toImageMsg();
			obs_publisher_->publish(*image_msg);
		}
	}
};

void master_ros() {
	printf("ROS_MASTER started\n\n\n");
	rclcpp::init(0, nullptr);
	//TODO: do the executorSafe nodes thign
	rclcpp::spin(std::make_shared<ObsActNode>());
	rclcpp::shutdown();
}
