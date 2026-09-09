// THIS IS MAIN FILE CONTROLLING THE VOXELS SIMULATION

#include "raylib.h"
#include "rlgl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "player_movement.hpp"
#include "god_movement.hpp"
#include "map.hpp"
#include "faces.hpp"
#include "master_voxel.hpp"
#include "support_for_master.hpp"
#include "small_handy_stuff.hpp"
#include "data_types.hpp"
#include "odometry.hpp"
#include "lidar.hpp"
#include "config_core.hpp"
#include "sim_params.hpp"
#include "world_config.hpp"
#include "world_loading.hpp"

// Draw an arrow at the agent's position pointing along its look direction.
// The body cube lives in vw->player_model and is built once in init_sim - it used to be
// generated and uploaded to the GPU on every single frame, which leaked a mesh per frame.
// IMPORTANT: never call this from inside the player_camera's own render pass.
static void drawPlayer(VoxelWorld *vw, Color color) {
	Camera3D camera = vw->player_camera;
	Vector3 forward = Vector3Subtract(camera.target, camera.position);
	forward.y = 0.0f;
	float len = sqrtf(forward.x * forward.x + forward.z * forward.z);
	if (len < 1e-3f) return;
	forward.x /= len;
	forward.z /= len;

	const float shaft_len = 0.7f;
	const float head_len  = 0.3f;
	const float shaft_r   = 0.15f;
	const float head_r    = 0.30f;

	Vector3 base = camera.position + forward * 0.5f;
	Vector3 shaft_end = (Vector3){
		base.x + forward.x * shaft_len,
		base.y,
		base.z + forward.z * shaft_len,
	};
	Vector3 tip = (Vector3){
		base.x + forward.x * (shaft_len + head_len),
		base.y,
		base.z + forward.z * (shaft_len + head_len),
	};

	// draw player cube
	DrawModelEx(
	    vw->player_model,
	    camera.position,
	    (Vector3){0,1,0},   // axis
	    getPlayerAngleDeg(camera),
	    (Vector3){1,1,1},
	    RED
	);

	// draw arrow to visualise player heading
	DrawCylinderEx(base, shaft_end, shaft_r, shaft_r, 8, color);
	DrawCylinderEx(shaft_end, tip,  head_r,  0.0f,    12, color);
}

// draw the voxel world itself - sky, ground and every chunk model.  Shared by the robot
// POV pass and the third-person pass so the two views can never diverge.
static void drawWorld(VoxelWorld *vw) {
	const Vector3 mazePosition = { 0.0f, 0.5f, 0.0f };

	DrawModel(vw->sky_model, (Vector3){0, 0, 0}, 1.0f, WHITE);		// draw the sky
	DrawModel(vw->ground_model, (Vector3){0, 0, 0}, 1.0f, WHITE);		// draw the ground

	// TODO get chunk_position of the ones surrounding the player and draw only them
	for (int i = 0; i < vw->maze_chunk_count; i++) {
		DrawModel(vw->maze_model[i], mazePosition, 1.0f, WHITE);
	}
}

// initilize simulation - allocate memory, create structs, define window size, etc.
// Window size, frame-rate cap, camera resolution and window visibility all come from
// sim_params (see sim_params.hpp) and must be set before this call.
VoxelWorld *init_sim(Vector3 player_pose, Vector3 player_direction) {

	VoxelWorld *vw = new VoxelWorld();

	if (!sim_params.verbose) SetTraceLogLevel(LOG_WARNING);
	if (!sim_params.show_window) SetConfigFlags(FLAG_WINDOW_HIDDEN);

	// start window
	InitWindow(sim_params.screen_width, sim_params.screen_height, "You're in voxels now.");
	SetTargetFPS(sim_params.target_fps > 0 ? sim_params.target_fps : 0);	// 0 removes the cap

	// setup camera
	Camera edit_camera = { 0 };
	edit_camera.position = (Vector3){ -10.0f, 40.0f, -10.0f };    // Camera position
	edit_camera.target = (Vector3){ -10.0f, 0.0f, 10.0f };    // Camera looking at point
	edit_camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };          // Camera up vector (rotation towards target)
	edit_camera.fovy = 45.0f;                                // Camera field-of-view Y
	edit_camera.projection = CAMERA_PERSPECTIVE;             // Camera projection type
	vw->edit_camera = edit_camera;

	// setup player
	Camera player_camera = { 0 };
	player_camera.position = player_pose;    // Camera position
	player_camera.target = {player_pose.x + player_direction.x, 0.5f, player_pose.z + player_direction.z};    // Camera looking at point
	player_camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };          // Camera up vector (rotation towards target)
	player_camera.fovy = 45.0f;                                // Camera field-of-view Y
	player_camera.projection = CAMERA_PERSPECTIVE;             // Camera projection type
	vw->player_camera = player_camera;

	// load which world to use from config file before building the world
	load_world_config("core_voxels/resources/worlds/worlds.config");
	vw->current_world = CurrentWorld;

	build_world(vw);	// map, chunk meshes/models, ground, sky - see world_loading.cpp

	// model of the player (robot) body - built once, reused every frame
	vw->player_model = LoadModelFromMesh(GenMeshCube(1, 1, 1));

	// mode variables
	vw->player_mode = false;
	vw->player_view = false;
	vw->current_camera = edit_camera;

	// initilize rest of VoxelWorld vw
	vw->camera_view_tex = LoadRenderTexture(sim_params.camera_width, sim_params.camera_height);
	vw->teleport_text.text_active = false;
	vw->teleport_text.letter_count = 0;
	vw->teleport_text.text_box = { (float)(sim_params.screen_width - 400), 100, 200, 50 };		// TODO: magic numbers
	vw->teleport_text.MAX_INPUT_CHARS = TELEPORT_TEXT_CAPACITY - 2;
	memset(vw->teleport_text.text, 0, sizeof(vw->teleport_text.text));
	if (sim_params.verbose) printf("init succesfull\n");
	return vw;
}

// this function is one step of simulation, here happen all calculations of what happens in the simulation as well as the actual visual rendering, called in loop from master_main
void step_sim(VoxelWorld *vw, Action *action, Observation *observation) {
	// TODO: move these somewhere a bit cleaner
	char position_info[70];
	char mode_info[70];

	// handle all keys pressed
	handleActionsAndKeys(vw, action, observation);	// movement based on keys stop movement on action until keys are released
	updateOdometry(vw, action, observation);	// currently twist msg inside odometry in observation is directly taken from action
	if (sim_params.lidar_enabled) updateLidar(vw, observation);		// cast LiDAR rays through the voxel grid

	// screenshot
	if (sim_params.keyboard_enabled && IsKeyPressed(KEY_SPACE)) {
		TakeScreenshot("screenshot.png");
	}

	// ---- robot front camera -------------------------------------------------
	// The single most expensive thing a step does, and always on: the image is the point
	// of the observation. Cost scales with camera_width * camera_height (sim_params).
	{
		BeginTextureMode(vw->camera_view_tex);
			ClearBackground(RAYWHITE);
			BeginMode3D(vw->player_camera);
				drawWorld(vw);
			EndMode3D();
		EndTextureMode();

		Image pov_view_img = LoadImageFromTexture(vw->camera_view_tex.texture);

		// Convert current robot POV view (front camera) from raylib Image to cv::Mat.
		// The OpenGL framebuffer origin is bottom-left, so the rows come back flipped;
		// cv::flip does that in the same pass that produces the output buffer, which
		// saves the separate ImageFlipVertical scan and the extra deep copy.
		cv::Mat rgba(pov_view_img.height, pov_view_img.width, CV_8UC4, pov_view_img.data);
		cv::Mat bgr;
		cv::cvtColor(rgba, bgr, cv::COLOR_RGBA2BGR);
		cv::flip(bgr, observation->camera_front, 0);

		UnloadImage(pov_view_img);
	}

	// ---- third-person / god view --------------------------------------------
	// BeginDrawing/EndDrawing always run: EndDrawing is what polls input and applies
	// the frame-rate cap, so skipping it would freeze the window and the keyboard.
	// Only the (expensive) second pass over the world is conditional.
	BeginDrawing();

		if (sim_params.render_gui) {
			ClearBackground(RAYWHITE);

			BeginMode3D(vw->current_camera);

				DrawModel(vw->sky_model, (Vector3){0, 0, 0}, 1.0f, WHITE);		// draw the sky
				DrawModel(vw->ground_model, (Vector3){0, 0, 0}, 1.0f, WHITE);		// draw the ground
				// Rays first, then agent, so the player marker stays readable over scan lines.
				if (!(vw->player_mode && vw->player_view)) {
					if (sim_params.lidar_enabled && sim_params.draw_lidar_rays) drawLidarRays(vw, observation, false);
					drawPlayer(vw, RED);				// draw heading arrow (the player)
				}

				// draw maze
				const Vector3 mazePosition = { 0.0f, 0.5f, 0.0f };
				for (int i = 0; i < vw->maze_chunk_count; i++) {	// draw voxels
					DrawModel(vw->maze_model[i], mazePosition, 1.0f, WHITE);
				}

				// for debugging: DrawGrid(1000, 1.0f);

				// TODO: draw cross in the middle of the screen (put toggle on/off in config)

			EndMode3D();

			// draw title, current position, fps
			DrawText("VOXELS", 10, 10, 30, BLACK);

			sprintf(position_info, "Current Player Position: x=%.2f, y=%.2f, z=%.2f", vw->player_camera.position.x, vw->player_camera.position.y, vw->player_camera.position.z);
			DrawText(position_info, 10, 50, 20, RED);

			sprintf(mode_info, "Player Mode? %d Player View? %d", vw->player_mode, vw->player_view);
			DrawText(mode_info, 10, 90, 20, BLACK);
			DrawFPS(10, 130);

			// draw teleport input box if T was pressed
			if (vw->teleport_text.text_active == true) {
				DrawRectangleRec(vw->teleport_text.text_box, (Color){0, 0, 0, 0});
				DrawRectangleLines((int)vw->teleport_text.text_box.x, (int)vw->teleport_text.text_box.y, (int)vw->teleport_text.text_box.width, (int)vw->teleport_text.text_box.height, DARKGRAY);
				DrawText(vw->teleport_text.text, (int)vw->teleport_text.text_box.x + 5, (int)vw->teleport_text.text_box.y + 8, 25, GREEN);
				DrawText(TextFormat("teleport: x z yaw_deg"), (int)vw->teleport_text.text_box.x + 5, vw->teleport_text.text_box.y + 40, 13, GREEN);
			}
		} else if (sim_params.show_window) {
			// GUI pass is off but the window is visible - blit the POV texture we already
			// rendered instead of leaving a stale frame on screen. One textured quad, so
			// it costs nothing next to a second pass over the world.
			DrawTexturePro(
				vw->camera_view_tex.texture,
				(Rectangle){ 0, 0, (float)vw->camera_view_tex.texture.width, -(float)vw->camera_view_tex.texture.height },
				(Rectangle){ 0, 0, (float)sim_params.screen_width, (float)sim_params.screen_height },
				(Vector2){ 0, 0 }, 0.0f, WHITE
			);
		}

	EndDrawing();
}

// tear everything down. Guarded on IsWindowReady() so calling it twice, or after the
// window has already gone, cannot touch a dead GL context.
void end_sim(VoxelWorld *vw) {
	const bool gl_alive = IsWindowReady();

	if (vw != nullptr) {
		if (gl_alive) {
			destroy_world(vw);
			UnloadModel(vw->player_model);
			UnloadRenderTexture(vw->camera_view_tex);
		} else {
			// no GL context left to release the GPU buffers through; free what is ours on the CPU
			delete[] vw->main_map.chunks;
			delete[] vw->maze_mesh;
			delete[] vw->maze_model;
		}
		delete vw;
	}

	if (gl_alive) CloseWindow();
}
