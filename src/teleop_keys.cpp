// TELEOP KEYS - for controlling the simulated robot trough ekys that are published as cmd_vel - to more resemble how real world robot is controled

#include <linux/input.h>
#include <fcntl.h>
#include <unistd.h>
#include <glob.h>

#include <cstdlib>
#include <fstream>
#include <regex>
#include <string>
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

namespace {

std::string first_glob_match(const char* pattern) {
	glob_t g{};
	std::string result;
	if (glob(pattern, 0, nullptr, &g) == 0 && g.gl_pathc > 0) {
		result = g.gl_pathv[0];
	}
	globfree(&g);
	return result;
}

std::string scan_proc_for_keyboard() {
	std::ifstream f("/proc/bus/input/devices");
	if (!f) return {};
	static const std::regex handlers_re(R"(^H: Handlers=(.*)$)");
	static const std::regex event_re(R"(event(\d+))");
	std::string line;
	while (std::getline(f, line)) {
		std::smatch m;
		if (!std::regex_match(line, m, handlers_re)) continue;
		const std::string handlers = m[1];
		if (handlers.find("kbd") == std::string::npos) continue;
		std::smatch em;
		if (std::regex_search(handlers, em, event_re)) {
			return "/dev/input/event" + em[1].str();
		}
	}
	return {};
}

std::string resolve_keyboard_device() {
	if (const char* env = std::getenv("TELEOP_KEYS_DEVICE")) {
		if (env[0] != '\0') return env;
	}
	if (auto p = first_glob_match("/dev/input/by-path/*-event-kbd"); !p.empty()) return p;
	if (auto p = first_glob_match("/dev/input/by-id/*-event-kbd"); !p.empty()) return p;
	if (auto p = scan_proc_for_keyboard(); !p.empty()) return p;
	throw std::runtime_error(
		"teleop_keys: could not find a keyboard input device. Tried "
		"$TELEOP_KEYS_DEVICE, /dev/input/by-path/*-event-kbd, "
		"/dev/input/by-id/*-event-kbd, and /proc/bus/input/devices. "
		"Set TELEOP_KEYS_DEVICE=/dev/input/eventN to override."
	);
}

} // namespace

TeleopKeysNode::TeleopKeysNode() : Node("teleop_keys"), running_(true) {
	// QoS set to match pfvtr (document this in the 'Extensive Documentation')
	auto cmd_qos = rclcpp::QoS(rclcpp::KeepLast(10)).best_effort();

	pub_ = this->create_publisher<geometry_msgs::msg::TwistStamped>("/cmd_vel_subscriber", cmd_qos);

	const std::string device_path = resolve_keyboard_device();
	device_fd_ = open(device_path.c_str(), O_RDONLY | O_NONBLOCK);
	if (device_fd_ < 0) {
		throw std::runtime_error(
			"teleop_keys: failed to open " + device_path +
			" (check permissions; user must be in the 'input' group). "
			"Set TELEOP_KEYS_DEVICE=/dev/input/eventN to override."
		);
	}
	RCLCPP_INFO(this->get_logger(), "teleop_keys: reading from %s", device_path.c_str());

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
			// Twist is published in ROS REP-103 frame: x forward, y left, z up.
			// The simulator's action_callback (ros_voxels.cpp) handles the
			// raylib axis swap via ros_axis_convert.hpp.
			msg.header.stamp = this->now();
			msg.header.frame_id = "base_link";

			// reset each spin
			msg.twist.linear.x = 0.0;
			msg.twist.linear.y = 0.0;
			msg.twist.angular.z = 0.0;

			// W/S = forward/back
			if (keys_[KEY_W]) msg.twist.linear.x = ROS_SPEED;
			if (keys_[KEY_S]) msg.twist.linear.x = -ROS_SPEED;

			// A/D = strafe (REP-103: +y is left)
			if (keys_[KEY_A]) msg.twist.linear.y = ROS_SPEED;
			if (keys_[KEY_D]) msg.twist.linear.y = -ROS_SPEED;

			// K/L = rotation
			if (keys_[KEY_K]) msg.twist.angular.z = ROS_TURN_SPEED;
			if (keys_[KEY_L]) msg.twist.angular.z = -ROS_TURN_SPEED;

			pub_->publish(msg);
		}
		rate.sleep();
	}
}

