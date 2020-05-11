#pragma once

#include <grend/engine.hpp>
#include <grend/sdl-context.hpp>
#include <grend/model.hpp>
#include <grend/glm-includes.hpp>
#include <grend/geometry-gen.hpp>
#include <grend/gl_manager.hpp>
#include <grend/utility.hpp>
#include <grend/octree.hpp>
#include <grend/text.hpp>
#include <grend/timers.hpp>
#include <grend/physics.hpp>

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

class camera {
	public:
		glm::vec3 position = glm::vec3(0);
		glm::vec3 velocity = glm::vec3(0);
		glm::vec3 direction = glm::vec3(0, 0, 1);

		glm::vec3 right = glm::vec3(1, 0, 0);
		glm::vec3 up = glm::vec3(0, 1, 0);

		void set_direction(glm::vec3 dir);
};

class game_editor {
	public:
		enum mode {
			Inactive,
			View,
			AddObject,
			AddLight,
			Select,
		};

		struct editor_entry {
			std::string name;
			glm::vec3   position;
			// TODO: make this a function
			glm::mat4   transform = glm::mat4(1);
			glm::vec3   scale = {1, 1, 1};
			// TODO: full rotation
			glm::quat   rotation = {0, 0, 0, 1};

			bool        inverted = false;
			bool        cull_backfaces = true;

			// TODO: might be a good idea to keep track of whether face culling is enabled for
			//       this model, although that might be better off in the model class itself...
		};

		void update_models(engine *renderer);
		void set_mode(enum mode newmode) { mode = newmode; };
		void handle_editor_input(engine *renderer, context& ctx, SDL_Event& ev);
		// TODO: rename 'engine' to 'renderer' or something
		void render_imgui(engine *renderer, context& ctx);
		void render_editor(engine *renderer, imp_physics *phys, context& ctx);
		void save_map(std::string name="save.map");
		void load_map(std::string name="save.map");
		void logic(context& ctx, float delta);

		int mode = mode::Inactive;
		/*
		glm::vec3 edit_position = glm::vec3(0, 0, 0);
		glm::mat4 edit_transform = glm::mat4(1);
		// this keeps track of the current face order after flipping along an axis
		bool      edit_inverted = false;
		*/

		float edit_distance = 5;
		editor_entry entbuf;

		camera cam;
		int selected_light = -1;
		int selected_object = -1;
		float movement_speed = 10.f;
		float fidelity = 10.f;
		float exposure = 1.f;
		float light_threshold = 0.03;

		// Map editing things
		std::vector<editor_entry> dynamic_models;
		gl_manager::cooked_model_map::const_iterator edit_model;

	private:
		void menubar(void);
		void map_models(engine *renderer, context& ctx);
		void map_window(engine *renderer, imp_physics *phys, context& ctx);
		void lights_window(engine *renderer, context& ctx);

		bool show_map_window = true;
		bool show_lights_window = false;
};

class game_state : public engine {
	friend class game_editor;

	public:
		game_state(context& ctx);
		virtual ~game_state();
		virtual void render(context& ctx);
		virtual void logic(context& ctx);
		virtual void physics(context& ctx);
		virtual void input(context& ctx);

		// TODO: subclasses or something
		void handle_player_input(SDL_Event& ev);

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

		glm::mat4 projection, view;

		/*
		// TODO: replace this with camera class
		glm::vec3 view_position = glm::vec3(0, 0, 0);
		glm::vec3 view_velocity;

		glm::vec3 view_direction = glm::vec3(0, 0, -1);
		glm::vec3 view_up = glm::vec3(0, 1, 0);
		glm::vec3 view_right = glm::vec3(1, 0, 0);
		*/

		camera player_cam;
		camera *current_cam = &player_cam;

		//glm::vec3 player_position = glm::vec3(0); // meters
		//glm::vec3 player_velocity = glm::vec3(0); // m/s
		uint64_t player_phys_id;
		glm::vec3 player_move_input = glm::vec3(0);
		glm::vec3 player_direction = glm::vec3(1, 0, 0);
		grendx::scene static_models;

		game_editor editor;

		// physics things
		//std::vector<physics_object> phys_objs;
		imp_physics phys;

		// sky box
		// TODO: should this be in the engine?
		gl_manager::rhandle skybox;
		gl_manager::rhandle skybox_shader;

		// main rendering shader
		gl_manager::rhandle main_shader;

		// post-processing shader
		gl_manager::rhandle post_shader;

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

		// testing stuff
		void draw_octree_leaves(octree::node *node, glm::vec3 location);
		octree oct;

		// text rendering
		text_renderer text;

		// FPS info
		sma_counter frame_timer;

	private:

};

// namespace grendx;
}
