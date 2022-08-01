#pragma once

#include <grend/engine.hpp>
#include <grend/renderPostStage.hpp>
#include <grend/sdlContext.hpp>
#include <grend/gameMain.hpp>
#include <grend/sceneNode.hpp>
#include <grend/sceneModel.hpp>
#include <grend/glmIncludes.hpp>
#include <grend/glManager.hpp>
#include <grend/utility.hpp>
#include <grend/physics.hpp>
#include <grend/camera.hpp>
#include <grend/gameView.hpp>
#include <grend/modalSDLInput.hpp>
#include <grend/TRS.hpp>
#include <grend/logger.hpp>

#include <grend/ecs/ecs.hpp>

namespace grendx {

class gameEditor : public gameView {
	public:
		typedef std::shared_ptr<gameEditor> ptr;
		typedef std::weak_ptr<gameEditor> weakptr;

		enum editAction {
			NewScene,
			Added,
			Moved,
			Scaled,
			Rotated,
			Deleted,
		};

		typedef std::function<void(sceneNode::ptr, editAction action)> editCallback;

		gameEditor(gameMain *game);
		renderPostChain::ptr post;
		renderPostStage<rOutput>::ptr loading_thing;
		Texture::ptr loading_img;

		virtual void handleEvent(gameMain *game, const SDL_Event& ev);
		virtual void render(gameMain *game, renderFramebuffer::ptr fb);
		virtual void update(gameMain *game, float delta);

		void initImgui(gameMain *game);
		void addEditorCallback(editCallback func);
		void runCallbacks(sceneNode::ptr node, editAction action);
		void clear(gameMain *game);

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
		int mode = mode::View;

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
		void renderImguiData(gameMain *game);
		void renderEditor(gameMain *game);
		void renderMapModels(gameMain *game);

		modelMap::const_iterator edit_model;
		sceneNode::ptr objects;
		sceneNode::ptr UIObjects;
		modelMap models;
		modelMap UIModels;

		sceneNode::ptr selectedNode = nullptr;
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
		void updateSelected(const TRS& updated);
		void renderWorldObjects(gameMain *game);
		void menubar(gameMain *game);
		void mapWindow(gameMain *game);
		void objectEditorWindow(gameMain *game);
		void objectSelectWindow(gameMain *game);
		void entityListWindow(gameMain *game);
		void entityEditorWindow(gameMain *game);
		void addEntityWindow(gameMain *game);
		void metricsWindow(gameMain *game);
		void profilerWindow(gameMain *game);
		void settingsWindow(gameMain *game);
		void logWindow(gameMain *game);

		// populates map object tree
		void addnodes(std::string name,
		              sceneNode::ptr obj,
		              std::set<sceneNode::ptr>& selectedPath);
		void addnodesRec(const std::string& name,
		                  sceneNode::ptr obj,
		                  std::set<sceneNode::ptr>& selectedPath);

		void handleMoveRotate(gameMain *game);
		void handleSelectObject(gameMain *game);
		void handleCursorUpdate(gameMain *game);
		void loadUIModels(void);
		void loadInputBindings(gameMain *game);
		void showLoadingScreen(gameMain *game);
		bool isUIObject(sceneNode::ptr obj);

		// TODO: utility function, should be move somewhere else
		sceneNode::ptr getNonModel(sceneNode::ptr obj);

		// list of log messages
		std::list<std::string> logEntries;

		std::vector<editCallback> callbacks;
		std::vector<std::pair<uint32_t, ecs::entity*>> clickState;

		modalSDLInput inputBinds;
		bool showMapWindow = false;
		bool showObjectEditorWindow = false;
		bool showObjectSelectWindow = false;
		bool showEntityEditorWindow = false;
		bool showEntityListWindow = false;
		bool showAddEntityWindow = false;
		bool showProfilerWindow = false;
		bool showMetricsWindow = true;
		bool showSettingsWindow = false;
		bool showLogWindow = true;

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
