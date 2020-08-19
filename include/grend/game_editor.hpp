#pragma once

#include <grend/engine.hpp>
#include <grend/sdl-context.hpp>
#include <grend/gameMain.hpp>
#include <grend/gameModel.hpp>
#include <grend/glm-includes.hpp>
#include <grend/gl_manager.hpp>
#include <grend/utility.hpp>
#include <grend/physics.hpp>
#include <grend/camera.hpp>
#include <grend/gameView.hpp>

namespace grendx {

class game_editor : public gameView {
	public:
		game_editor(gameMain *game) : gameView() {
			initImgui(game);
		};

		virtual void handleInput(gameMain *game, SDL_Event& ev);
		virtual void render(gameMain *game);
		void initImgui(gameMain *game);

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
			std::string classname = "<default>";
			glm::vec3   position;
			// TODO: make this a function
			glm::mat4   transform = glm::mat4(1);
			glm::vec3   scale = {1, 1, 1};
			// NOTE: glm quat contructor is (w, x, y, z)
			glm::quat   rotation = {1, 0, 0, 0};

			bool        inverted = false;
			bool        cull_backfaces = true;

			// TODO: might be a good idea to keep track of whether face culling is enabled for
			//       this model, although that might be better off in the model class itself...
		};

		void load_model(gameMain *game, std::string path);
		void load_scene(gameMain *game, std::string path);
		void update_models(gameMain *game);
		void reload_shaders(gameMain *game);
		void set_mode(enum mode newmode) { mode = newmode; };
		// TODO: rename 'engine' to 'renderer' or something
		void render_imgui(gameMain *game);
		void render_editor(gameMain *game);
		void render_map_models(gameMain *game);

		void save_map(gameMain *game, std::string name="save.map");
		void load_map(gameMain *game, std::string name="save.map");
		void logic(float delta);
		void clear(gameMain *game);

		int mode = mode::Inactive;

		float edit_distance = 5;
		editor_entry entbuf;

		camera::ptr cam = camera::ptr(new camera());
		gameObject::ptr selectedNode = nullptr;
		float movement_speed = 10.f;
		float fidelity = 10.f;
		float exposure = 1.f;
		float light_threshold = 0.03;

		// Map editing things
		std::vector<editor_entry> dynamic_models;
		// list of imported files
		std::vector<std::string> editor_model_files;
		std::vector<std::string> editor_scene_files;

		cooked_model_map::const_iterator edit_model;

	private:
		void menubar(gameMain *game);
		void map_window(gameMain *game);
		void lights_window(gameMain *game);
		void refprobes_window(gameMain *game);
		void object_select_window(gameMain *game);

		bool show_map_window = false;
		bool show_lights_window = false;
		bool show_refprobe_window = false;
		bool show_object_select_window = false;
};

// namespace grendx
}
