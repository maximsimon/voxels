#include <chrono>
#include <memory>
#include <string>
#include <cstdlib>

#include "raylib.h"
#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"
#include "std_msgs/msg/int32.hpp"
#include "sensor_msgs/msg/image.hpp"
#include "nav_msgs/msg/odometry.hpp"
#include "geometry_msgs/msg/twist.hpp"
#include <tf2_ros/transform_broadcaster.h>
#include <geometry_msgs/msg/transform_stamped.hpp>

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
		camera_publisher_ = this->create_publisher<sensor_msgs::msg::Image>("camera_front_publisher", 10);		// publisher that fetces observation from voxel world simulation and publishes it to topic, for outside programs to see inside the simulation trough ROS
		odom_publisher_ = this->create_publisher<nav_msgs::msg::Odometry>("odometry_publisher", 10);		// publisher that fetces observation from voxel world simulation and publishes it to topic, for outside programs to see inside the simulation trough ROS
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
	rclcpp::Publisher<sensor_msgs::msg::Image>::SharedPtr camera_publisher_;
	rclcpp::Publisher<nav_msgs::msg::Odometry>::SharedPtr odom_publisher_;
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
			// image
			image_msg = cv_bridge::CvImage(std_msgs::msg::Header(), "bgr8", observation_->camera_front).toImageMsg();
			camera_publisher_->publish(*image_msg);
			
			// odom
			// NEXT_STEP: this does not woooork, why? throws segmentation fault, also TODO: orientation needs to be in quaternions so figure it out (probably in simulation, not here - not sure yet where ,decide
			
			// odom msg created every publish with should be fine, using msg shared_ptr like image worked badly with threads (segmentation fault)
			nav_msgs::msg::Odometry odom_msg;
			
			odom_msg.header.stamp = this->now();	
			
			odom_msg.header.frame_id = "odom";
			odom_msg.child_frame_id = "base_link"; // robot frame
		
			// mismateched x,y,z and w,x because raylib uses different axis convention than ROS	
			odom_msg.pose.pose.position.x = -observation_->position.z;
			odom_msg.pose.pose.position.y = -observation_->position.x;
			odom_msg.pose.pose.position.z = observation_->position.y;

			odom_msg.pose.pose.orientation.w = observation_->orientation.x;
			odom_msg.pose.pose.orientation.x = observation_->orientation.w;
			odom_msg.pose.pose.orientation.y = observation_->orientation.y;
			odom_msg.pose.pose.orientation.z = observation_->orientation.z;

			odom_msg.twist.twist.linear.x = -action_->linear_vel.z;
			odom_msg.twist.twist.linear.y = -action_->linear_vel.x;
			odom_msg.twist.twist.linear.z = action_->linear_vel.y;
			
			odom_msg.twist.twist.angular.x = action_->angular_vel.x;
			odom_msg.twist.twist.angular.y = action_->angular_vel.y;
			odom_msg.twist.twist.angular.z = action_->angular_vel.z;
			
			odom_publisher_->publish(odom_msg);

			//experiment with TF2
			tf2_ros::TransformBroadcaster tf_broadcaster_(this);

			geometry_msgs::msg::TransformStamped t;
			t.header.stamp = this->now();           // current ROS2 time
			t.header.frame_id = "odom";             // world frame
			t.child_frame_id = "base_link";         // robot frame
			t.transform.translation.x = -observation_->position.z;
			t.transform.translation.y = -observation_->position.x;
			t.transform.translation.z = observation_->position.y;
			t.transform.rotation.w = observation_->orientation.x; 
			t.transform.rotation.x = observation_->orientation.w; 
			t.transform.rotation.y = observation_->orientation.y; 
			t.transform.rotation.z = observation_->orientation.z; 

			tf_broadcaster_.sendTransform(t);
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
