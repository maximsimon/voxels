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

using namespace std::chrono_literals;

class ObsActNode : public rclcpp::Node
{
public:
	ObsActNode()
	: Node("obs_act_node")
	{
		camera_publisher_ = this->create_publisher<sensor_msgs::msg::Image>("camera_front_publisher", 10);		// publisher that fetces observation from voxel world simulation and publishes it to topic, for outside programs to see inside the simulation trough ROS
		odom_publisher_ = this->create_publisher<nav_msgs::msg::Odometry>("odometry_publisher", 10);		// publisher that fetces observation from voxel world simulation and publishes it to topic, for outside programs to see inside the simulation trough ROS
		cmd_vel_publisher_ = this->create_publisher<geometry_msgs::msg::Twist>("cmd_vel_publisher", 10);		// publisher of robots movement (twist msg - linear and angular velocitie), yes that info is already in odom_publsher, but cmd_vel is traiditionally on robots and bearnav needs this topic for creating a map
		cmd_vel_subscriber_ = this->create_subscription<geometry_msgs::msg::Twist>(
		    "cmd_vel_subscriber",
		    10,
		    std::bind(&ObsActNode::action_callback, this, std::placeholders::_1)
		);	// subscriber that gets action from outside and passes it to voxel world simulation, for outside programs to control an agent in the simulation trough ROS
	
		int step_size = 2;
		// Timer to call publish_every_spin() every 100ms
		timer_ = this->create_wall_timer(
			std::chrono::milliseconds(50),
			std::bind(&ObsActNode::publish_every_spin, this)
        	);

	}

private:
	rclcpp::TimerBase::SharedPtr timer_;
	rclcpp::Publisher<sensor_msgs::msg::Image>::SharedPtr camera_publisher_;
	rclcpp::Publisher<nav_msgs::msg::Odometry>::SharedPtr odom_publisher_;
	rclcpp::Publisher<geometry_msgs::msg::Twist>::SharedPtr cmd_vel_publisher_;
	rclcpp::Subscription<geometry_msgs::msg::Twist>::SharedPtr cmd_vel_subscriber_;
	sensor_msgs::msg::Image::SharedPtr image_msg;
	Observation* observation_ = new Observation();
	Action* action_ = new Action();
	rclcpp::Time action_last_msg;

	void action_callback(const geometry_msgs::msg::Twist::SharedPtr cmd_vel_msg) {
		action_->linear_vel = {(float)cmd_vel_msg->linear.x, (float)cmd_vel_msg->linear.y, (float)cmd_vel_msg->linear.z};
		action_->angular_vel = {(float)cmd_vel_msg->angular.x, (float)cmd_vel_msg->angular.y, (float)cmd_vel_msg->angular.z};
	
		////TODO: figure out how to streamline fetching obseervation (every callback? every spin? both? figure it out)
	
		if (action_ != NULL) {
			master_ros_bridge(action_, observation_);
		}

		action_last_msg = rclcpp::Clock().now();
	}

	void publish_every_spin() {
		master_ros_bridge(action_, observation_);
		if (observation_ != NULL) {
			// image
			image_msg = cv_bridge::CvImage(std_msgs::msg::Header(), "bgr8", observation_->camera_front).toImageMsg();
			image_msg->header.stamp = this->now();
			image_msg->header.frame_id = "camera_front_publish";
			camera_publisher_->publish(*image_msg);
			
			// odom
			// odom msg created every publish with should be fine, using msg shared_ptr like image worked badly with threads (segmentation fault)
			nav_msgs::msg::Odometry odom_msg;
			
			odom_msg.header.stamp = this->now();	
			
			odom_msg.header.frame_id = "odom";
			odom_msg.child_frame_id = "base_link"; // robot frame
		
			// possiblly mismateched x,y,z and w,x when displayed in rvix or something, because raylib uses different axis convention than ROS	
			odom_msg.pose.pose.position.x = observation_->position.x;
			odom_msg.pose.pose.position.y = observation_->position.y;
			odom_msg.pose.pose.position.z = observation_->position.z;

			odom_msg.pose.pose.orientation.w = observation_->orientation.w;
			odom_msg.pose.pose.orientation.x = observation_->orientation.x;
			odom_msg.pose.pose.orientation.y = observation_->orientation.y;
			odom_msg.pose.pose.orientation.z = observation_->orientation.z;

			odom_msg.twist.twist.linear.x = action_->linear_vel.x;
			odom_msg.twist.twist.linear.y = action_->linear_vel.y;
			odom_msg.twist.twist.linear.z = action_->linear_vel.z;
			
			odom_msg.twist.twist.angular.x = action_->angular_vel.x;
			odom_msg.twist.twist.angular.y = action_->angular_vel.y;
			odom_msg.twist.twist.angular.z = action_->angular_vel.z;
			
			odom_publisher_->publish(odom_msg);

			// twist publish for bearnav mapmaker	
			geometry_msgs::msg::Twist cmd_vel_out_msg;

			cmd_vel_out_msg.linear.x = observation_->linear_vel.x;
			cmd_vel_out_msg.linear.y = observation_->linear_vel.y;
			cmd_vel_out_msg.linear.z = observation_->linear_vel.z;

			cmd_vel_out_msg.angular.x = observation_->angular_vel.x;
			cmd_vel_out_msg.angular.y = observation_->angular_vel.y;
			cmd_vel_out_msg.angular.z = observation_->angular_vel.z;
            
			cmd_vel_publisher_->publish(cmd_vel_out_msg);
		
			// tf2 for visualisations in rviz and such
			tf2_ros::TransformBroadcaster tf_broadcaster_(this);

			geometry_msgs::msg::TransformStamped t;
			t.header.stamp = this->now();           // current ROS2 time
			t.header.frame_id = "odom";             // world frame
			t.child_frame_id = "base_link";         // robot frame
			t.transform.translation.x = observation_->position.x;
			t.transform.translation.y = observation_->position.y;
			t.transform.translation.z = observation_->position.z;
			t.transform.rotation.w = observation_->orientation.w; 
			t.transform.rotation.x = observation_->orientation.x; 
			t.transform.rotation.y = observation_->orientation.y; 
			t.transform.rotation.z = observation_->orientation.z; 

			tf_broadcaster_.sendTransform(t);

			if ((rclcpp::Clock().now() - action_last_msg).seconds() > 0.2) {
				action_->linear_vel.x = 0.0;	
				action_->linear_vel.y = 0.0;	
				action_->linear_vel.z = 0.0;	
				
				action_->angular_vel.x = 0.0;	
				action_->angular_vel.y = 0.0;	
				action_->angular_vel.z = 0.0;	
			}

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
