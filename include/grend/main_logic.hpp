#pragma once

#include <grend/engine.hpp>
#include <grend/sdl-context.hpp>
#include <grend/model.hpp>
#include <grend/glm-includes.hpp>
#include <grend/geometry-gen.hpp>
#include <grend/gl_manager.hpp>
#include <grend/utility.hpp>
#include <grend/text.hpp>
#include <grend/timers.hpp>
#include <grend/physics.hpp>
#include <grend/camera.hpp>
#include <grend/game_editor.hpp>

#include <tuple>
#include <vector>
#include <fstream>
#include <iostream>
#include <sstream>
#include <iterator>
#include <string>
#include <exception>
#include <memory>
#include <utility>
#include <map>
#include <list>

namespace grendx {

class game_state {
	friend class game_editor;
	friend class renderer;

	public:
		game_state(context& ctx);
		virtual ~game_state();
		virtual void render(context& ctx);
		virtual void logic(context& ctx);
		virtual void physics(context& ctx);
		virtual void input(context& ctx);

		// TODO: subclasses or something
		void handle_player_input(SDL_Event& ev);

		// TODO: move some rendering stuff into the engine code
		void render_light_maps(context& ctx);
		void render_light_info(context& ctx);
		void render_skybox(context& ctx);
		void render_static(context& ctx);
		void render_players(context& ctx);
		void render_dynamic(context& ctx);
		void render_postprocess(context& ctx);

		void load_models(void);
		void load_shaders(void);
		void init_framebuffers(void);
		void init_test_lights(void);
		void init_imgui(context& ctx);

		void draw_debug_string(std::string str);

		bool running;
		renderer rend;
		game_editor editor;
		imp_physics phys;
		text_renderer text;
		// FPS info
		sma_counter frame_timer;

		camera player_cam;
		camera *current_cam = &player_cam;

		glm::mat4 projection, view;

		uint64_t player_phys_id;
		glm::vec3 player_move_input = glm::vec3(0);
		glm::vec3 player_direction = glm::vec3(1, 0, 0);
		grendx::scene static_models;

		// sky box
		// TODO: should this be in the engine?
		gl_manager::rhandle skybox;
		gl_manager::rhandle skybox_shader;

		// main rendering shader
		gl_manager::rhandle main_shader;

		// post-processing shader
		gl_manager::rhandle post_shader;

		// simplified lighting probes for calculating reflection/shadow maps
		gl_manager::rhandle refprobe_shader;
		gl_manager::rhandle lightprobe_shader;
		gl_manager::rhandle shadow_shader;

		// debug shaders for light/reflection probes
		gl_manager::rhandle refprobe_debug;

		// main rendering framebuffer
		gl_manager::rhandle rend_fb;
		gl_manager::rhandle rend_tex;
		gl_manager::rhandle rend_depth;
		int rend_x, rend_y; // framebuffer dimensions

		// previous frame info
		Uint32 last_frame;
		gl_manager::rhandle last_frame_fb; // same dimensions as rend_fb
		gl_manager::rhandle last_frame_tex; // same dimensions as rend_fb

		// dynamic resolution scaling
		void adjust_draw_resolution(void);
		float dsr_target_fps = 60.0;
		float dsr_target_ms = 1000/dsr_target_fps;
		float dsr_scale_x = 1.0;
		float dsr_scale_y = 1.0;
		/*
		// TODO: use this once I figure out how to do vsync
		float dsr_scale_down = dsr_target_ms * 0.90;
		float dsr_scale_up = dsr_target_ms * 0.80;
		*/
		float dsr_scale_down = dsr_target_ms * 1.1;
		float dsr_scale_up = dsr_target_ms * 1.03;
		float dsr_min_scale_x = 0.50;
		float dsr_min_scale_y = 0.50;
		float dsr_down_incr = 0.10;
		float dsr_up_incr = 0.01;

		// (actual) screen size
		int screen_x, screen_y;

		// dynamic lights
		int player_light;

	private:

};

// namespace grendx;
}
