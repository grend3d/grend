#pragma once

#include <grend/engine.hpp>
#include <grend/renderPostStage.hpp>
#include <grend/sdlContext.hpp>
#include <grend/gameMain.hpp>
#include <grend/gameObject.hpp>
#include <grend/gameModel.hpp>
#include <grend/glmIncludes.hpp>
#include <grend/glManager.hpp>
#include <grend/utility.hpp>
#include <grend/physics.hpp>
#include <grend/camera.hpp>
#include <grend/gameView.hpp>
#include <grend/modalSDLInput.hpp>
#include <grend/TRS.hpp>

namespace grendx {

std::pair<gameObject::ptr, modelMap> loadModel(std::string path);
std::pair<gameImport::ptr, modelMap> loadSceneData(std::string path);
gameImport::ptr loadSceneCompiled(std::string path);
// TODO: should this return an {object, future} pair?
//       could also just check for an asyncLoaded node, that's probably fine
gameImport::ptr loadSceneAsyncCompiled(gameMain *game, std::string path);

void saveMap(gameMain *game,
			 gameObject::ptr root,
			 std::string name="save.map");
std::pair<gameObject::ptr, modelMap> loadMapData(gameMain *game,
                                                 std::string name="save.map");
gameObject::ptr loadMapCompiled(gameMain *game, std::string name="save.map");
gameObject::ptr loadMapAsyncCompiled(gameMain *game, std::string name="save.map");

class gameEditor : public gameView {
	public:
		gameEditor(gameMain *game);
		renderPostChain::ptr post;
		renderPostStage<rOutput>::ptr loading_thing;
		Texture::ptr loading_img;

		virtual void handleInput(gameMain *game, SDL_Event& ev);
		virtual void render(gameMain *game);
		void initImgui(gameMain *game);

		enum mode {
			Inactive,
			View,
			AddSomething,
			AddObject,
			AddPointLight,
			AddSpotLight,
			AddDirectionalLight,
			AddReflectionProbe,
			AddIrradianceProbe,
			Select,
			MoveSomething,
			MoveX,
			MoveY,
			MoveZ,
			RotateSomething,
			RotateX,
			RotateY,
			RotateZ,
			ScaleSomething,
			ScaleX,
			ScaleY,
			ScaleZ,
			MoveAABBPosX,
			MoveAABBPosY,
			MoveAABBPosZ,
			MoveAABBNegX,
			MoveAABBNegY,
			MoveAABBNegZ,
		};

		// TODO: don't need this anymore
		struct editorEntry {
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

		void updateModels(gameMain *game);
		void reloadShaders(gameMain *game);
		void setMode(enum mode newmode);
		// TODO: rename 'engine' to 'renderer' or something
		void renderImgui(gameMain *game);
		void renderEditor(gameMain *game);
		void renderMapModels(gameMain *game);

		void logic(gameMain *game, float delta);
		void clear(gameMain *game);

		int mode = mode::View;
		modelMap::const_iterator edit_model;
		gameObject::ptr objects;
		gameObject::ptr UIObjects;
		modelMap models;
		modelMap UIModels;

		gameObject::ptr selectedNode = nullptr;
		ecs::entity *selectedEntity = nullptr;

		float movementSpeed = 10.f;
		float fidelity = 10.f;
		float exposure = 1.f;
		float lightThreshold = 0.03;
		float editDistance = 5;

		TRS cursorBuf;
		TRS transformBuf;

		// Map editing things
		// TODO: don't need dynamic_models anymore
		std::vector<editorEntry> dynamic_models;

	private:
		void renderWorldObjects(gameMain *game);
		void menubar(gameMain *game);
		void mapWindow(gameMain *game);
		void objectEditorWindow(gameMain *game);
		void objectSelectWindow(gameMain *game);
		void entityEditorWindow(gameMain *game);
		void entitySelectWindow(gameMain *game);
		void addEntityWindow(gameMain *game);
		void metricsWindow(gameMain *game);
		void profilerWindow(gameMain *game);
		void settingsWindow(gameMain *game);

		// populates map object tree
		void addnodes(std::string name,
		              gameObject::ptr obj,
		              std::set<gameObject::ptr>& selectedPath);
		void addnodesRec(const std::string& name,
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
		bool showMapWindow = false;
		bool showObjectEditorWindow = false;
		bool showObjectSelectWindow = false;
		bool showEntityEditorWindow = false;
		bool showEntitySelectWindow = false;
		bool showAddEntityWindow = false;
		bool showProfilerWindow = false;
		bool showMetricsWindow = true;
		bool showSettingsWindow = true;
		bool showProbes = true;
		bool showLights = true;

		// last place the mouse was clicked, used for determining the amount of
		// rotation, movement etc
		// (position / width, height)
		float clickedX, clickedY;
		// distance from cursor to camera at the last click
		float clickDepth;
};

// namespace grendx
}
