#pragma once

#include <grend/engine.hpp>
#include <grend/sdl-context.hpp>
#include <grend/model.hpp>
#include <grend/glm-includes.hpp>
#include <grend/gl_manager.hpp>
#include <grend/utility.hpp>
#include <grend/physics.hpp>
#include <grend/camera.hpp>

namespace grendx {

class game_editor {
	public:
		enum mode {
			Inactive,
			View,
			AddObject,
			AddPointLight,
			AddSpotLight,
			AddDirectionalLight,
			AddReflectionProbe,
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

		void load_model(engine *renderer, std::string path);
		void update_models(engine *renderer);
		void set_mode(enum mode newmode) { mode = newmode; };
		void handle_editor_input(engine *renderer, context& ctx, SDL_Event& ev);
		// TODO: rename 'engine' to 'renderer' or something
		void render_imgui(engine *renderer, context& ctx);
		void render_editor(engine *renderer, imp_physics *phys, context& ctx);
		void render_map_models(engine *renderer, context& ctx);

		void save_map(engine *renderer, std::string name="save.map");
		void load_map(engine *renderer, std::string name="save.map");
		void logic(context& ctx, float delta);
		void clear(engine *renderer);

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
		int selected_refprobe = -1;
		float movement_speed = 10.f;
		float fidelity = 10.f;
		float exposure = 1.f;
		float light_threshold = 0.03;

		// Map editing things
		std::vector<editor_entry> dynamic_models;
		// list of imported files
		std::vector<std::string> editor_model_files;

		// Keep track of lights added in the editor so that we can export
		// them to the save file later (don't want other dynamic/effect lights
		// to be exported/imported)
		struct {
			std::vector<uint32_t> point;
			std::vector<uint32_t> spot;
			std::vector<uint32_t> directional;
		} edit_lights;
		gl_manager::cooked_model_map::const_iterator edit_model;

	private:
		void menubar(engine *renderer);
		void map_window(engine *renderer, imp_physics *phys, context& ctx);
		void lights_window(engine *renderer, context& ctx);
		void refprobes_window(engine *renderer, context& ctx);

		bool show_map_window = false;
		bool show_lights_window = false;
		bool show_refprobe_window = false;
};

// namespace grendx
}
