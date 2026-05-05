#include <chrono>
#include <memory>
#include <string>
#include <cstdlib>

#include "raylib.h"
#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"
#include "std_msgs/msg/int32.hpp"
#include "sensor_msgs/msg/image.hpp"
#include "sensor_msgs/msg/camera_info.hpp"
#include "nav_msgs/msg/odometry.hpp"
#include "geometry_msgs/msg/twist.hpp"
#include "geometry_msgs/msg/twist_stamped.hpp"
#include "geometry_msgs/msg/pose_with_covariance_stamped.hpp"
#include <tf2_ros/transform_broadcaster.h>
#include <geometry_msgs/msg/transform_stamped.hpp>
#include <cmath>

#include <opencv2/opencv.hpp>
#include <cv_bridge/cv_bridge.hpp>

#include "ros_voxels.hpp"
#include "teleop_keys.hpp"
#include "data_types.hpp"
#include "master_main.hpp"

using namespace std::chrono_literals;

class MasterRosNode : public rclcpp::Node
{
public:
	MasterRosNode()
	: Node("master_ros_node")
	{
		auto sensor_qos = rclcpp::QoS(rclcpp::KeepLast(5)).best_effort();
		auto cmd_qos = rclcpp::QoS(rclcpp::KeepLast(10)).best_effort();

		camera_publisher_ = this->create_publisher<sensor_msgs::msg::Image>("camera_front_publisher", sensor_qos);
		// Standard ROS image_pipeline convention: CameraInfo lives at <image_topic>/camera_info.
		// ROS2 enforces one type per topic name, so this MUST be a different topic from the image.
		camera_info_publisher_ = this->create_publisher<sensor_msgs::msg::CameraInfo>("camera_front_publisher/camera_info", sensor_qos);
		odom_publisher_ = this->create_publisher<nav_msgs::msg::Odometry>("odometry_publisher", sensor_qos);
		cmd_vel_publisher_ = this->create_publisher<geometry_msgs::msg::TwistStamped>("cmd_vel_publisher", sensor_qos);
		cmd_vel_subscriber_ = this->create_subscription<geometry_msgs::msg::TwistStamped>(
		    "cmd_vel_subscriber",
		    cmd_qos,
		    std::bind(&MasterRosNode::action_callback, this, std::placeholders::_1)
		);

		// Subscribe to the rviz "2D Pose Estimate" convention so external
		// tools (Tk GUI, rviz, scripts) can teleport the robot.  We use the
		// standard ROS REP-103 convention here: pose.position.{x,y} are the
		// floor plane (z up), yaw is the rotation about +Z extracted from
		// the quaternion.  The simulation thread re-maps to its raylib axes
		// (y up) when applying the teleport.
		rclcpp::QoS pose_qos = rclcpp::QoS(rclcpp::KeepLast(1)).reliable();
		teleport_subscriber_ = this->create_subscription<geometry_msgs::msg::PoseWithCovarianceStamped>(
		    "/initialpose",
		    pose_qos,
		    std::bind(&MasterRosNode::teleport_callback, this, std::placeholders::_1)
		);

		// 20 ms period = 50 Hz, matching the simulation's render rate
		// (SetTargetFPS(100/step_size) with step_size=2 in master_main.cpp).
		timer_ = this->create_wall_timer(
			std::chrono::milliseconds(20),
			std::bind(&MasterRosNode::publish_every_spin, this)
        	);

	}

private:
	rclcpp::TimerBase::SharedPtr timer_;
	rclcpp::Publisher<sensor_msgs::msg::Image>::SharedPtr camera_publisher_;
	rclcpp::Publisher<sensor_msgs::msg::CameraInfo>::SharedPtr camera_info_publisher_;
	sensor_msgs::msg::CameraInfo camera_info_;
	rclcpp::Publisher<nav_msgs::msg::Odometry>::SharedPtr odom_publisher_;
	rclcpp::Publisher<geometry_msgs::msg::TwistStamped>::SharedPtr cmd_vel_publisher_;
	rclcpp::Subscription<geometry_msgs::msg::TwistStamped>::SharedPtr cmd_vel_subscriber_;
	rclcpp::Subscription<geometry_msgs::msg::PoseWithCovarianceStamped>::SharedPtr teleport_subscriber_;
	sensor_msgs::msg::Image::SharedPtr image_msg;
	Observation* observation_ = new Observation();
	Action* action_ = new Action();
	rclcpp::Time action_last_msg;

	void teleport_callback(const geometry_msgs::msg::PoseWithCovarianceStamped::SharedPtr msg) {
		// Map ROS REP-103 floor (x forward, y left, z up) to the sim's
		// raylib axes (x along +x, z along +z floor plane, y up).  Only x/y
		// of the pose are used; height stays pinned at 0.5 inside the sim.
		const double px = msg->pose.pose.position.x;
		const double py = msg->pose.pose.position.y;
		const double qw = msg->pose.pose.orientation.w;
		const double qx = msg->pose.pose.orientation.x;
		const double qy = msg->pose.pose.orientation.y;
		const double qz = msg->pose.pose.orientation.z;

		// Yaw about +Z extracted from quaternion (tf2 formula).
		const double yaw = std::atan2(2.0 * (qw * qz + qx * qy),
		                              1.0 - 2.0 * (qy * qy + qz * qz));

		// ROS x → raylib x (forward floor coord);
		// ROS y → raylib z (lateral floor coord).
		// Raylib's getPlayerAngle is atan2(z, x); ROS yaw is the same angle
		// in the same X-Z plane after the swap above, so pass yaw straight
		// through.
		master_ros_teleport(static_cast<float>(px),
		                    static_cast<float>(py),
		                    static_cast<float>(yaw));

		RCLCPP_INFO(this->get_logger(),
		            "teleport request: x=%.2f y=%.2f yaw=%.2f rad",
		            px, py, yaw);
	}

	void action_callback(const geometry_msgs::msg::TwistStamped::SharedPtr cmd_vel_msg) {
		//cmd_vel_msg_msg.header.stamp = this->now();
		//cmd_vel_msg.header.frame_id = "base_link";
		action_->linear_vel = {(float)cmd_vel_msg->twist.linear.x, (float)cmd_vel_msg->twist.linear.y, (float)cmd_vel_msg->twist.linear.z};
		// ROS REP-103 convention: angular.z is yaw rate. The sim internally
		// stores yaw in angular_vel.y (raylib is Y-up), so translate at the
		// boundary. x/z (roll/pitch) are unused by the 2D sim.
		action_->angular_vel = {0.0f, (float)cmd_vel_msg->twist.angular.z, 0.0f};
	
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

			// CameraInfo paired with the image (raylib fovy=45°, square pixels, no distortion).
			// Static intrinsics computed once on first frame; per-frame cost is just a header stamp.
			// Published on <image_topic>/camera_info (standard image_pipeline convention).
			if (camera_info_.width == 0) {
				camera_info_.width = image_msg->width;
				camera_info_.height = image_msg->height;
				double f = image_msg->height / (2.0 * std::tan(M_PI / 8));
				camera_info_.k = {f, 0, image_msg->width / 2.0, 0, f, image_msg->height / 2.0, 0, 0, 1};
			}
			camera_info_.header = image_msg->header;
			camera_info_publisher_->publish(camera_info_);
			
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
			
			// Sim's internal yaw lives in angular_vel.y; emit it on angular.z
			// per ROS REP-103 so downstream nodes see standard conventions.
			odom_msg.twist.twist.angular.x = 0.0;
			odom_msg.twist.twist.angular.y = 0.0;
			odom_msg.twist.twist.angular.z = action_->angular_vel.y;
			
			odom_publisher_->publish(odom_msg);

			// twist publish for bearnav mapmaker
			geometry_msgs::msg::TwistStamped cmd_vel_out_msg;
			cmd_vel_out_msg.header.stamp = this->now();
			cmd_vel_out_msg.header.frame_id = "base_link";

			cmd_vel_out_msg.twist.linear.x = observation_->linear_vel.x;
			cmd_vel_out_msg.twist.linear.y = observation_->linear_vel.y;
			cmd_vel_out_msg.twist.linear.z = observation_->linear_vel.z;

			// Publish yaw on angular.z (ROS REP-103). Internal convention
			// keeps yaw in observation_->angular_vel.y (raylib Y-up).
			cmd_vel_out_msg.twist.angular.x = 0.0;
			cmd_vel_out_msg.twist.angular.y = 0.0;
			cmd_vel_out_msg.twist.angular.z = observation_->angular_vel.y;

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
	printf("ROS_MASTER started\n\n");
	rclcpp::init(0, nullptr);
	//TODO: do the executorSafe nodes thign
	//rclcpp::spin(std::make_shared<MasterRosNode>());
	//rclcpp::spin(std::make_shared<MasterRosNode>());

	auto master_ros = std::make_shared<MasterRosNode>();
	auto teleop_keys = std::make_shared<TeleopKeysNode>();

	rclcpp::executors::MultiThreadedExecutor exec;
	exec.add_node(master_ros);
	exec.add_node(teleop_keys);

	exec.spin();
	
	rclcpp::shutdown();
}
