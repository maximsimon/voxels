// PYTHON BRIDGE - exposes the Voxel World simulation to Python via pybind11.
//
//   import voxel_sim
//   obs = voxel_sim.Init()                      # returns dict with first observation
//   while obs["running"]:
//       obs = voxel_sim.Step((1.0, 0.0, 0.0, 0.0, 0.0, 0.0))   # [vx,vy,vz, wx,wy,wz]
//   voxel_sim.Close()
//
// observation dict keys:
//   camera_front : np.uint8[H, W, 3]  BGR image from the robot front camera
//   position     : np.float32[3]      x, y, z
//   orientation  : np.float32[4]      w, x, y, z
//   linear_vel   : np.float32[3]
//   angular_vel  : np.float32[3]
//   lidar_scan   : np.float32[60]     ranges in meters
//   running      : bool               false once the window/quit key requests exit

#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>

#include <cstring>

#include "master_main.hpp"

namespace py = pybind11;

// pack a raylib Vector3 into a numpy float32[3] array
static py::array_t<float> vec3_to_array(float x, float y, float z) {
	py::array_t<float> arr(3);
	float *p = arr.mutable_data();
	p[0] = x; p[1] = y; p[2] = z;
	return arr;
}

// pack a raylib Quaternion into a numpy float32[4] array (w, x, y, z)
static py::array_t<float> quat_to_array(const Quaternion &q) {
	py::array_t<float> arr(4);
	float *p = arr.mutable_data();
	p[0] = q.w; p[1] = q.x; p[2] = q.y; p[3] = q.z;
	return arr;
}

// pack the latest public observation (master_observation) into a python dict
static py::dict pack_observation(bool running) {
	py::dict obs;

	const cv::Mat &cam = master_observation.camera_front;
	if (cam.empty()) {
		obs["camera_front"] = py::none();
	} else {
		// camera is BGR8, stored row-major -> np.uint8[H, W, 3]
		py::array_t<uint8_t> arr({cam.rows, cam.cols, 3});
		std::memcpy(arr.mutable_data(), cam.data, cam.total() * cam.elemSize());
		obs["camera_front"] = arr;
	}

	obs["position"] = vec3_to_array(master_observation.position.x,
	                                master_observation.position.y,
	                                master_observation.position.z);
	obs["orientation"] = quat_to_array(master_observation.orientation);
	obs["linear_vel"] = vec3_to_array(master_observation.linear_vel.x,
	                                  master_observation.linear_vel.y,
	                                  master_observation.linear_vel.z);
	obs["angular_vel"] = vec3_to_array(master_observation.angular_vel.x,
	                                   master_observation.angular_vel.y,
	                                   master_observation.angular_vel.z);

	py::array_t<float> lidar(NUM_LIDAR_RAYS);
	std::memcpy(lidar.mutable_data(), master_observation.lidar_scan,
	            NUM_LIDAR_RAYS * sizeof(float));
	obs["lidar_scan"] = lidar;

	obs["running"] = running;
	return obs;
}

// initializes the simulation once and returns the first observation.
// call once per process (a second Init leaks the previous simulation state).
static py::dict Init() {
	master_init_sim();
	bool running = master_step_sim();	// one idle step -> first populated observation
	return pack_observation(running);
}

// steps the simulation with the given action and returns the new observation.
// action: sequence of 6 floats [vx, vy, vz, wx, wy, wz] (velocities m/s and rad/s)
static py::dict Step(py::array_t<float, py::array::c_style | py::array::forcecast> action) {
	const py::buffer_info info = action.request();
	if (info.ndim != 1 || info.shape[0] != 6) {
		throw std::invalid_argument(
			"action must be a 1-D sequence of 6 floats [vx, vy, vz, wx, wy, wz]");
	}
	const float *a = static_cast<const float *>(info.ptr);

	master_action.linear_vel.x  = a[0];
	master_action.linear_vel.y  = a[1];
	master_action.linear_vel.z  = a[2];
	master_action.angular_vel.x = a[3];
	master_action.angular_vel.y = a[4];
	master_action.angular_vel.z = a[5];

	bool running = master_step_sim();
	return pack_observation(running);
}

// queues a teleport request, applied at the next step.
// x, z are floor-plane coords; yaw_deg is the heading angle in degrees from +x
static void Teleport(float x, float z, float yaw_deg) {
	master_ros_teleport(x, z, yaw_deg * (PI / 180.0f));
}

static void Close() {
	CloseWindow();
}

PYBIND11_MODULE(voxel_sim, m) {
	m.doc() =
		"Voxel World robot simulator. Call Init() once, then Step(action) in a "
		"loop: action is 6 floats [vx,vy,vz, wx,wy,wz], each call returns an "
		"observation dict (camera_front BGR image, pose, velocities, lidar_scan, "
		"running). Keyboard controls (P/WASD/K/L/V/Q) still work in the window.";

	m.def("Init", &Init,
	      "initialize the simulation and return the first observation");
	m.def("Step", &Step, py::arg("action"),
	      "step the simulation with a 6-float action and return the observation");
	m.def("Teleport", &Teleport, py::arg("x"), py::arg("z"), py::arg("yaw_deg"),
	      "queue a teleport to floor coords (x, z) with heading yaw_deg (applied next step)");
	m.def("Close", &Close, "close the simulation window");

	// constants useful on the python side
	m.attr("NUM_LIDAR_RAYS") = NUM_LIDAR_RAYS;
	m.attr("MAX_LIDAR_RANGE") = MAX_LIDAR_RANGE;
}