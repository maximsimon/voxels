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
#include "ros_axis_convert.hpp"
#include "teleop_keys.hpp"
#include "data_types.hpp"
#include "master_main.hpp"

using ros_voxels::rl_pos_to_ros;
using ros_voxels::rl_vec_to_ros;
using ros_voxels::rl_quat_to_ros;
using ros_voxels::ros_vec_to_rl;

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

		// /initialpose carries an rviz "2D Pose Estimate" in ROS REP-103.
		// Axis conversion to raylib is delegated to ros_axis_convert.hpp.
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
		// Pose floor coords (px, py) are passed straight to master_ros_teleport,
		// which already takes them in raylib floor (x, z) layout — i.e. the
		// y<->z swap happens implicitly at the function-signature boundary.
		// Height (ROS z) is unused; the sim pins the camera at y=0.5.
		const double px = msg->pose.pose.position.x;
		const double py = msg->pose.pose.position.y;
		const double qw = msg->pose.pose.orientation.w;
		const double qx = msg->pose.pose.orientation.x;
		const double qy = msg->pose.pose.orientation.y;
		const double qz = msg->pose.pose.orientation.z;

		// Yaw about +Z extracted from the ROS quaternion (tf2 formula).
		// Raylib yaw == ROS yaw under the codebase's axis convention.
		const double yaw = std::atan2(2.0 * (qw * qz + qx * qy),
		                              1.0 - 2.0 * (qy * qy + qz * qz));

		master_ros_teleport(static_cast<float>(px),
		                    static_cast<float>(py),
		                    static_cast<float>(yaw));

		RCLCPP_INFO(this->get_logger(),
		            "teleport request: x=%.2f y=%.2f yaw=%.2f rad",
		            px, py, yaw);
	}

	void action_callback(const geometry_msgs::msg::TwistStamped::SharedPtr cmd_vel_msg) {
		// Incoming twist is ROS REP-103; ros_vec_to_rl performs the y<->z swap
		// so raylib slots line up with what support_for_master.cpp expects:
		//   linear_vel.x = forward, linear_vel.z = lateral, angular_vel.y = yaw.
		action_->linear_vel  = ros_vec_to_rl(cmd_vel_msg->twist.linear);
		action_->angular_vel = ros_vec_to_rl(cmd_vel_msg->twist.angular);

		if (action_ != NULL) {
			master_ros_bridge(action_, observation_);
		}

		action_last_msg = rclcpp::Clock().now();
	}

	void publish_every_spin() {
		master_ros_bridge(action_, observation_);
		if (observation_ != NULL) {
			// Build ALL messages first, then publish back-to-back at the end.
			// Reason: with every publish() the underlying RMW serialises and
			// hands off to the kernel — non-trivial wall-clock cost. If we
			// interleave message construction between publishes, the actual
			// publish() calls land at different wall-clock instants, and the
			// downstream subscribers see them spread out (which made the
			// synchronizer in mapmaker miss matches under Kilted/Fast DDS 3.x).
			// Using a single stamp keeps message timestamps identical; doing
			// the publishes back-to-back keeps wall-clock arrival close too.

			const rclcpp::Time stamp = this->now();
			const std_msgs::msg::Header img_header = [&]() {
				std_msgs::msg::Header h;
				h.stamp = stamp;
				h.frame_id = "camera_front_publish";
				return h;
			}();

			// --- BUILD: image -----------------------------------------------
			image_msg = cv_bridge::CvImage(std_msgs::msg::Header(), "bgr8", observation_->camera_front).toImageMsg();
			image_msg->header = img_header;

			// --- BUILD: camera_info (intrinsics lazy-init from first image) -
			if (camera_info_.width == 0) {
				camera_info_.width = image_msg->width;
				camera_info_.height = image_msg->height;
				double f = image_msg->height / (2.0 * std::tan(M_PI / 8));
				camera_info_.k = {f, 0, image_msg->width / 2.0, 0, f, image_msg->height / 2.0, 0, 0, 1};
			}
			camera_info_.header = img_header;

			// --- BUILD: odom ------------------------------------------------
			nav_msgs::msg::Odometry odom_msg;
			odom_msg.header.stamp = stamp;
			odom_msg.header.frame_id = "odom";
			odom_msg.child_frame_id = "base_link";
			odom_msg.pose.pose.position    = rl_pos_to_ros(observation_->position);
			odom_msg.pose.pose.orientation = rl_quat_to_ros(observation_->orientation);
			odom_msg.twist.twist.linear    = rl_vec_to_ros(action_->linear_vel);
			odom_msg.twist.twist.angular   = rl_vec_to_ros(action_->angular_vel);

			// --- BUILD: cmd_vel (for bearnav mapmaker) ----------------------
			geometry_msgs::msg::TwistStamped cmd_vel_out_msg;
			cmd_vel_out_msg.header.stamp = stamp;
			cmd_vel_out_msg.header.frame_id = "base_link";
			cmd_vel_out_msg.twist.linear  = rl_vec_to_ros(observation_->linear_vel);
			cmd_vel_out_msg.twist.angular = rl_vec_to_ros(observation_->angular_vel);

			// --- BUILD: tf transform ----------------------------------------
			tf2_ros::TransformBroadcaster tf_broadcaster_(this);
			geometry_msgs::msg::TransformStamped t;
			t.header.stamp = stamp;
			t.header.frame_id = "odom";
			t.child_frame_id = "base_link";
			t.transform.translation = rl_vec_to_ros(observation_->position);
			t.transform.rotation    = rl_quat_to_ros(observation_->orientation);

			// --- PUBLISH ALL: back-to-back, no work in between --------------
			camera_publisher_->publish(*image_msg);
			camera_info_publisher_->publish(camera_info_);
			odom_publisher_->publish(odom_msg);
			cmd_vel_publisher_->publish(cmd_vel_out_msg);
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
