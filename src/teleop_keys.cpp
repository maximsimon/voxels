// TELEOP KEYS - for controlling the simulated robot trough ekys that are published as cmd_vel - to more resemble how real world robot is controled

#include <linux/input.h>
#include <fcntl.h>
#include <unistd.h>

#include <thread>
#include <atomic>
#include <unordered_map>

#include "rclcpp/rclcpp.hpp"
#include "geometry_msgs/msg/twist_stamped.hpp"

#include "teleop_keys.hpp"

// control how fast the robot moves or turns based on keyboard input
float ROS_SPEED = 0.1;
float ROS_TURN_SPEED = 0.02;

// TODO: make this so that it can be at least turned off if not automatically off, WHEN YOU ARE NOT IN THE SIMULATION WINDOW OR SOMWTHING, for example when my ism is running and im writing somewhere 'publisher' and player mode turns on and then it reads all keys and moves the robot

TeleopKeysNode::TeleopKeysNode() : Node("teleop_keys"), running_(true) {
	// QoS set to match pfvtr (document this in the 'Extensive Documentation')
	auto cmd_qos = rclcpp::QoS(rclcpp::KeepLast(10)).best_effort();
	
	pub_ = this->create_publisher<geometry_msgs::msg::TwistStamped>("/cmd_vel_subscriber", cmd_qos);

        // CHANGE THIS to your device
        device_fd_ = open("/dev/input/event4", O_RDONLY | O_NONBLOCK);
        if (device_fd_ < 0) {
		throw std::runtime_error("Failed to open /dev/input/eventX (check permissions)");
		close(device_fd_); 
	}

        thread_ = std::thread(&TeleopKeysNode::loop, this);
}

TeleopKeysNode::~TeleopKeysNode() {
        running_ = false;

        if (thread_.joinable())
            thread_.join();

        if (device_fd_ >= 0)
            close(device_fd_);
}



void TeleopKeysNode::loop() {
	rclcpp::Rate rate(50);

	struct input_event ev;
	geometry_msgs::msg::TwistStamped msg;

	bool player_mode = false;

	while (rclcpp::ok() && running_) {
	// -------- READ ALL EVENTS --------

		while (read(device_fd_, &ev, sizeof(ev)) > 0) {
			if (ev.type == EV_KEY) {
				// 1 = press, 0 = release
				keys_[ev.code] = (ev.value != 0);
			}
		}
	// ssize_t n = read(device_fd_, &ev, sizeof(ev));

	// if (n == sizeof(ev))
	// {
	// if (ev.type == EV_KEY)
	// {
	//     keys_[ev.code] = (ev.value != 0);
	// }
	// }
	// else if (n == -1)
	// {
	// // EAGAIN → no input, normal
	// }

		// P to turn on player_mode, O to turn off player_mode
		if (keys_[KEY_P]) player_mode = true;
		if (keys_[KEY_O]) player_mode = false;
		// publish only if in player mode (it overides other source that publish to /cmd_vel_subscirbe to control the orbot)	TODO: actually make it as a hardcore overide like the button on ps4 controllers for robot + fix this clumsy o and p tur on or off (only p detects it too often and can change it to 0 and back to 1 on 1 p press

		if (player_mode == true) {
			// -------- BUILD CMD_VEL --------
			//header
			msg.header.stamp = this->now();
			msg.header.frame_id = "base_link";
			
			// TODO: sort out PERMANENTLY the x, y, z ROS to x, z, y RAYLIB coord mishmash

			// reset each spin
			msg.twist.linear.x = 0.0;
			msg.twist.linear.z = 0.0;
			msg.twist.angular.z = 0.0;
			
			// W/S = forward/back
			if (keys_[KEY_W]) msg.twist.linear.x = ROS_SPEED;
			if (keys_[KEY_S]) msg.twist.linear.x = -ROS_SPEED;

			// A/D = strafe
			if (keys_[KEY_A]) msg.twist.linear.z = -ROS_SPEED;
			if (keys_[KEY_D]) msg.twist.linear.z = ROS_SPEED;

			// K/L = rotation
			if (keys_[KEY_K]) msg.twist.angular.z = ROS_TURN_SPEED;
			if (keys_[KEY_L]) msg.twist.angular.z = -ROS_TURN_SPEED;

			pub_->publish(msg);
		}
		rate.sleep();
	}
}

