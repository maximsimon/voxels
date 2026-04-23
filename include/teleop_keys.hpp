// TELEOP KEYS - for controlling the simulated robot trough ekys that are published as cmd_vel - to more resemble how real world robot is controled

#ifndef TELEOP_KEYS_H
#define TELEOP_KEYS_H

#include <map>
#include <vector>
#include <thread>
#include <atomic>
#include <termios.h>

#include "rclcpp/rclcpp.hpp"
#include "geometry_msgs/msg/twist_stamped.hpp"



class TeleopKeysNode : public rclcpp::Node
{
public:
    TeleopKeysNode();
    ~TeleopKeysNode();

private:
	
	void loop();
	
	//void keyboardLoop();
	//void setupTerminal();
	//void restoreTerminal();

	//int getch();
	
	rclcpp::Publisher<geometry_msgs::msg::TwistStamped>::SharedPtr pub_;

	int device_fd_;
	std::thread thread_;
	std::atomic<bool> running_;
	
	// key state map (THIS is the important part)
	std::unordered_map<int, bool> keys_;

	// player_mode controls if player is controlled by keys or not (equivalent to simulations vw->player_mode
	bool player_mode;
};

#endif
