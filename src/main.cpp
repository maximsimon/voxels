// PYTHON BRIDGE - exposes the Voxel World simulation to Python via pybind11.
//
//   import voxel_sim
//   voxel_sim.Configure(target_fps=0, render_gui=False)   # optional, before Init()
//   obs = voxel_sim.Init()                      # returns dict with first observation
//   while obs["running"]:
//       obs = voxel_sim.Step((1.0, 0.0, 0.0, 0.0, 0.0, 0.0))   # [vx,vy,vz, wx,wy,wz]
//   obs = voxel_sim.Reset("ConferenceWorld", (0.0, 0.0, 90.0)) # world + (x, z, yaw_deg)
//   voxel_sim.Close()
//
// observation dict keys:
//   camera_front : np.uint8[H, W, 3]  BGR image from the robot front camera
//   position     : np.float32[3]      x, y, z
//   orientation  : np.float32[4]      w, x, y, z
//   yaw          : float               floor-plane heading from +x, radians
//   linear_vel   : np.float32[3]
//   angular_vel  : np.float32[3]
//   lidar_scan   : np.float32[N]      ranges in meters, N = configured lidar_rays
//   running      : bool               false once the window/quit key requests exit

#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>

#include <cstring>
#include <stdexcept>
#include <string>

#include "master_main.hpp"
#include "sim_params.hpp"
#include "world_config.hpp"
#include "world_registry.hpp"

namespace py = pybind11;

static bool g_initialized = false;

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
		// only before the first step has rendered anything
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
	obs["yaw"] = master_observation.yaw;		// radians, floor-plane heading from +x
	obs["linear_vel"] = vec3_to_array(master_observation.linear_vel.x,
	                                  master_observation.linear_vel.y,
	                                  master_observation.linear_vel.z);
	obs["angular_vel"] = vec3_to_array(master_observation.angular_vel.x,
	                                   master_observation.angular_vel.y,
	                                   master_observation.angular_vel.z);

	// only the configured number of rays carries a real range; the rest of the fixed-size
	// buffer is padding, so hand python exactly the slice that means something
	const int rays = simActiveLidarRays();
	py::array_t<float> lidar(rays);
	std::memcpy(lidar.mutable_data(), master_observation.lidar_scan, rays * sizeof(float));
	obs["lidar_scan"] = lidar;

	obs["running"] = running;
	return obs;
}

// ---------------------------------------------------------------------------
// Configure - the runtime knobs from sim_params.hpp, as python keyword arguments.
// ---------------------------------------------------------------------------

static void require_positive(const char *name, int value) {
	if (value <= 0) throw std::invalid_argument(std::string(name) + " must be > 0");
}

static void Configure(const py::kwargs &kwargs) {
	for (auto item : kwargs) {
		const std::string key = py::str(item.first);
		const py::handle value = item.second;

		// INIT-ONLY knobs: reading them after Init() would silently do nothing, so say so
		const bool init_only = (key == "show_window" || key == "screen_width" ||
		                        key == "screen_height" || key == "camera_width" ||
		                        key == "camera_height");
		if (init_only && g_initialized) {
			throw std::invalid_argument(
				"'" + key + "' can only be set before Init() - it is read once when the window is created");
		}

		if      (key == "fixed_dt")          sim_params.fixed_dt = value.cast<float>();
		else if (key == "target_fps")        sim_params.target_fps = value.cast<int>();
		else if (key == "show_window")       sim_params.show_window = value.cast<bool>();
		else if (key == "render_gui")        sim_params.render_gui = value.cast<bool>();
		else if (key == "screen_width")    { sim_params.screen_width = value.cast<int>();  require_positive("screen_width", sim_params.screen_width); }
		else if (key == "screen_height")   { sim_params.screen_height = value.cast<int>(); require_positive("screen_height", sim_params.screen_height); }
		else if (key == "camera_width")    { sim_params.camera_width = value.cast<int>();  require_positive("camera_width", sim_params.camera_width); }
		else if (key == "camera_height")   { sim_params.camera_height = value.cast<int>(); require_positive("camera_height", sim_params.camera_height); }
		else if (key == "lidar_enabled")     sim_params.lidar_enabled = value.cast<bool>();
		else if (key == "lidar_rays")        sim_params.lidar_rays = value.cast<int>();
		else if (key == "lidar_range")       sim_params.lidar_range = value.cast<float>();
		else if (key == "draw_lidar_rays")   sim_params.draw_lidar_rays = value.cast<bool>();
		else if (key == "fast_collisions")    sim_params.fast_collisions = value.cast<bool>();
		else if (key == "collision_radius")   sim_params.collision_radius = value.cast<float>();
		else if (key == "max_linear_speed")  sim_params.max_linear_speed = value.cast<float>();
		else if (key == "max_angular_speed") sim_params.max_angular_speed = value.cast<float>();
		else if (key == "keyboard_enabled")  sim_params.keyboard_enabled = value.cast<bool>();
		else if (key == "quit_key_enabled")  sim_params.quit_key_enabled = value.cast<bool>();
		else if (key == "verbose")           sim_params.verbose = value.cast<bool>();
		else throw std::invalid_argument("unknown parameter '" + key + "' - see voxel_sim.GetConfig() for the accepted names");
	}
}

static py::dict GetConfig() {
	py::dict cfg;
	cfg["fixed_dt"] = sim_params.fixed_dt;
	cfg["target_fps"] = sim_params.target_fps;
	cfg["show_window"] = sim_params.show_window;
	cfg["render_gui"] = sim_params.render_gui;
	cfg["screen_width"] = sim_params.screen_width;
	cfg["screen_height"] = sim_params.screen_height;
	cfg["camera_width"] = sim_params.camera_width;
	cfg["camera_height"] = sim_params.camera_height;
	cfg["lidar_enabled"] = sim_params.lidar_enabled;
	cfg["lidar_rays"] = simActiveLidarRays();
	cfg["lidar_range"] = sim_params.lidar_range;
	cfg["draw_lidar_rays"] = sim_params.draw_lidar_rays;
	cfg["fast_collisions"] = sim_params.fast_collisions;
	cfg["collision_radius"] = sim_params.collision_radius;
	cfg["max_linear_speed"] = sim_params.max_linear_speed;
	cfg["max_angular_speed"] = sim_params.max_angular_speed;
	cfg["keyboard_enabled"] = sim_params.keyboard_enabled;
	cfg["quit_key_enabled"] = sim_params.quit_key_enabled;
	cfg["verbose"] = sim_params.verbose;
	return cfg;
}

// ---------------------------------------------------------------------------
// simulation lifecycle
// ---------------------------------------------------------------------------

// initializes the simulation once and returns the first observation.
// call once per process (a second Init leaks the previous simulation state).
static py::dict Init() {
	if (g_initialized) throw std::runtime_error("Init() has already been called in this process");
	master_init_sim();
	g_initialized = true;
	bool running = master_step_sim();	// one idle step -> first populated observation
	return pack_observation(running);
}

// steps the simulation with the given action and returns the new observation.
// action: sequence of 6 floats [vx, vy, vz, wx, wy, wz] (velocities m/s and rad/s)
static py::dict Step(py::array_t<float, py::array::c_style | py::array::forcecast> action) {
	if (!g_initialized) throw std::runtime_error("Step() called before Init()");

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

// Reset the episode: optionally switch world, put the robot at a chosen pose, and return
// a fresh observation.
//   world    - any registered world name (predefined or loaded from the registry file),
//              or None to stay in the world already loaded.
//   position - (x, z) or (x, z, yaw_deg) on the floor plane; None keeps the current pose.
// The world is only rebuilt when it actually changes, so resetting inside one world costs
// little more than a step.
static py::dict Reset(py::object world, py::object position) {
	if (!g_initialized) throw std::runtime_error("Reset() called before Init()");

	std::string world_name;
	if (!world.is_none()) {
		world_name = world.cast<std::string>();
		if (!world_name.empty() && world_name != master_current_world_name()) {
			World probe;
			if (!world_by_name(world_name.c_str(), &probe)) {
				throw std::invalid_argument("unknown world '" + world_name + "' - known worlds: " +
				                            std::string(world_names()));
			}
		}
	}

	// default to standing still exactly where the robot already is
	float x = master_observation.position.x;
	float z = master_observation.position.z;
	float yaw_rad = master_observation.yaw;

	if (!position.is_none()) {
		const std::vector<float> p = position.cast<std::vector<float>>();
		if (p.size() != 2 && p.size() != 3) {
			throw std::invalid_argument("position must be (x, z) or (x, z, yaw_deg)");
		}
		x = p[0];
		z = p[1];
		if (p.size() == 3) yaw_rad = p[2] * (PI / 180.0f);
	}

	if (!master_reset_sim(world_name.empty() ? nullptr : world_name.c_str(), x, z, yaw_rad)) {
		throw std::runtime_error("reset failed - the simulation is not initialized");
	}

	bool running = master_step_sim();	// one idle step -> observation at the new pose
	return pack_observation(running);
}

// queues a teleport request, applied at the next step.
// x, z are floor-plane coords; yaw_deg is the heading angle in degrees from +x
static void Teleport(float x, float z, float yaw_deg) {
	master_main_teleport(x, z, yaw_deg * (PI / 180.0f));
}

static void Close() {
	if (!g_initialized) return;
	master_end_sim();
	g_initialized = false;
}

static std::string CurrentWorldName() {
	return master_current_world_name();
}

// hue type -> palette color name, for dumping a world's color coding
static const char *hue_name_of(HUE_TYPE type) {
	switch (type) {
		case white:  return "white";
		case red:    return "red";
		case orange: return "orange";
		case yellow: return "yellow";
		case green:  return "green";
		case cyan:   return "cyan";
		case blue:   return "blue";
		case purple: return "purple";
		case pink:   return "pink";
		case brown:  return "brown";
		default:     return "unknown";
	}
}

// full definition of one registered world; name=None means the currently loaded world.
static py::dict WorldInfo(py::object name) {
	py::dict info;
	World world;

	if (name.is_none()) {
		std::string current = master_current_world_name();
		if (current.empty()) return info;	// nothing loaded yet (before Init)
		if (!registry_world_by_name(current.c_str(), &world)) return info;
	} else {
		const std::string wanted = name.cast<std::string>();
		if (!registry_world_by_name(wanted.c_str(), &world)) {
			throw std::invalid_argument("unknown world '" + wanted + "' - known worlds: " +
			                            registry_world_names());
		}
	}

	info["name"] = registry_world_name_of(world);
	info["map"] = world.MAP_IMAGE_PATH ? world.MAP_IMAGE_PATH : "";
	info["atlas"] = world.TEXTURE_ATLAS_PATH ? world.TEXTURE_ATLAS_PATH : "";
	info["ground"] = world.GROUND_TEXTURE_PATH ? world.GROUND_TEXTURE_PATH : "";
	info["sky"] = world.SKY_TEXTURE_PATH ? world.SKY_TEXTURE_PATH : "";
	info["atlas_cols"] = world.atlas_cols;
	info["atlas_rows"] = world.atlas_rows;
	py::list colors;
	for (const HueClass &cls : world.hue_config.colors) colors.append(hue_name_of(cls.type));
	info["colors"] = colors;
	return info;
}

// every registered world by name - whatever the registry currently holds
static void refresh_worlds(py::module_ &m) {
	py::list worlds;
	for (int i = 0; i < registry_world_count(); i++) worlds.append(registry_world_name_at(i));
	m.attr("WORLDS") = py::tuple(worlds);
}

// clear the world registry and replace it with the file; refresh WORLDS so the tuple stays accurate.
static void ReloadRegistry(const std::string &path) {
	load_world_registry(path.c_str());
	py::module_ m = py::module_::import("voxel_sim");
	refresh_worlds(m);
}

PYBIND11_MODULE(voxel_sim, m) {
	// load the world registry at import time, so WORLDS is populated before Init() builds the sim
	load_world_config("core_voxels/resources/worlds/worlds.config");

	m.doc() =
		"Voxel World robot simulator. Optionally Configure(...), then call Init() once "
		"and Step(action) in a loop: action is 6 floats [vx,vy,vz, wx,wy,wz], each call "
		"returns an observation dict (camera_front BGR image, pose, velocities, "
		"lidar_scan, running). Reset(world, position) restarts an episode. Keyboard "
		"controls (P/WASD/K/L/V/Q) work in the window unless keyboard_enabled=False.";


	m.def("Configure", &Configure,
	      "set runtime parameters as keyword arguments; see GetConfig() for the names.\n"
	      "Speed-relevant ones: target_fps=0 (uncapped), fixed_dt (simulated seconds per\n"
	      "step), render_gui=False (skip the third-person pass), smaller camera_width/\n"
	      "camera_height (the front camera dominates step cost, ~3 ns per pixel),\n"
	      "fast_collisions=True, show_window=False (never map the window),\n"
	      "keyboard_enabled=False (deterministic rollouts).\n"
	      "show_window, screen_* and camera_* must be set before Init().");
	m.def("GetConfig", &GetConfig, "current values of every runtime parameter, as a dict");

	m.def("Init", &Init,
	      "initialize the simulation and return the first observation");
	m.def("Step", &Step, py::arg("action"),
	      "step the simulation with a 6-float action and return the observation");
	m.def("Reset", &Reset, py::arg("world") = py::none(), py::arg("position") = py::none(),
	      "reset the episode: switch to `world` (None keeps the current one) and place the\n"
	      "robot at `position` = (x, z) or (x, z, yaw_deg) (None keeps the current pose).\n"
	      "Returns the observation at the new pose.");
	m.def("Teleport", &Teleport, py::arg("x"), py::arg("z"), py::arg("yaw_deg"),
	      "queue a teleport to floor coords (x, z) with heading yaw_deg (applied next step)");
	m.def("Close", &Close, "close the simulation and release the world and the window");
	m.def("ReloadRegistry", &ReloadRegistry, py::arg("path"),
	      "clear the world registry and replace it with the file; refresh WORLDS so the tuple stays accurate");
	m.def("CurrentWorld", &CurrentWorldName, "name of the world currently loaded");

	// dump a whole registered world so scripts do not need a getter per field.
	// name=None returns the currently loaded world (call after Init()).
	m.def("WorldInfo", &WorldInfo, py::arg("name") = py::none(),
	      "full definition of a registered world as a dict: name, map, atlas, ground, sky,\n"
	      "atlas_cols, atlas_rows and the colors that world maps pixels to.  name=None gives\n"
	      "the world currently loaded (call after Init()).");

	// lowercase alias, so the module reads naturally next to gym-style code
	m.attr("reset") = m.attr("Reset");

	// constants useful on the python side
	m.attr("NUM_LIDAR_RAYS") = NUM_LIDAR_RAYS;	// compile-time capacity of the scan buffer
	m.attr("MAX_LIDAR_RANGE") = MAX_LIDAR_RANGE;	// build-time default for lidar_range

	refresh_worlds(m);
}
