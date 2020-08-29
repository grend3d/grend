#pragma once

#include <grend/engine.hpp>
#include <grend/sdl-context.hpp>
#include <grend/gameMain.hpp>
#include <grend/gameObject.hpp>
#include <grend/gameModel.hpp>
#include <grend/glm-includes.hpp>
#include <grend/gl_manager.hpp>
#include <grend/utility.hpp>
#include <grend/physics.hpp>
#include <grend/camera.hpp>
#include <grend/gameView.hpp>
#include <grend/modalSDLInput.hpp>

namespace grendx {

void saveMap(gameMain *game,
			 gameObject::ptr root,
			 std::string name="save.map");
gameObject::ptr loadMap(gameMain *game, std::string name="save.map");

class game_editor : public gameView {
	public:
		game_editor(gameMain *game);
		renderPostStage<rOutput>::ptr testpost;
		renderPostStage<rOutput>::ptr loading_thing;
		Texture::ptr loading_img;

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
			MoveX,
			MoveY,
			MoveZ,
			RotateX,
			RotateY,
			RotateZ,
		};

		// TODO: don't need this anymore
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
		void set_mode(enum mode newmode);
		// TODO: rename 'engine' to 'renderer' or something
		void render_imgui(gameMain *game);
		void render_editor(gameMain *game);
		void render_map_models(gameMain *game);

		void logic(gameMain *game, float delta);
		void clear(gameMain *game);

		int mode = mode::View;
		model_map::const_iterator edit_model;
		gameObject::ptr objects;
		gameObject::ptr UI_objects;
		model_map models;
		model_map UI_models;

		camera::ptr cam = camera::ptr(new camera());
		gameObject::ptr selectedNode = nullptr;
		float movement_speed = 10.f;
		float fidelity = 10.f;
		float exposure = 1.f;
		float light_threshold = 0.03;
		float edit_distance = 5;
		editor_entry entbuf;

		// Map editing things
		// TODO: don't need dynamic_models anymore
		std::vector<editor_entry> dynamic_models;

	private:
		void menubar(gameMain *game);
		void map_window(gameMain *game);
		void objectEditorWindow(gameMain *game);
		void object_select_window(gameMain *game);
		// populates map object tree
		void addnodes(std::string name, gameObject::ptr obj);
		void addnodes_rec(const std::string& name,
		                  gameObject::ptr obj,
		                  std::set<gameObject::ptr>& selectedPath);

		void handleMoveRotate(gameMain *game);
		void handleSelectObject(gameMain *game);
		void handleCursorUpdate(gameMain *game);
		void loadUIModels(void);
		void loadInputBindings(gameMain *game);
		void showLoadingScreen(gameMain *game);
		bool isUIObject(gameObject::ptr obj);

		// TODO: utility function, should be move somewhere else
		gameObject::ptr getNonModel(gameObject::ptr obj);

		modalSDLInput inputBinds;
		bool show_map_window = false;
		bool show_object_editor_window = false;
		bool show_object_select_window = false;

		// last place the mouse was clicked, used for determining the amount of
		// rotation, movement etc
		// (position / width, height)
		float clicked_x, clicked_y;
		// distance from cursor to camera at the last click
		float click_depth;
};

// namespace grendx
}
