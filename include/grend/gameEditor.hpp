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
#include <grend/filePane.hpp>

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

		gameEditor();
		renderPostChain::ptr post;
		renderPostStage<rOutput>::ptr loading_thing;
		Texture::ptr loading_img;

		virtual void handleEvent(const SDL_Event& ev);
		virtual void render(renderFramebuffer::ptr fb);
		virtual void update(float delta);

		void initImgui();
		void addEditorCallback(editCallback func);
		void runCallbacks(sceneNode::ptr node, editAction action);
		void clear();

		bool showProbes = true;
		bool showLights = true;

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

		void updateModels();
		void reloadShaders();
		void setMode(enum mode newmode);
		// TODO: rename 'engine' to 'renderer' or something
		void renderMapModels();

		modelMap::const_iterator edit_model;
		sceneNode::ptr objects;
		sceneNode::ptr UIObjects;
		modelMap models;
		modelMap UIModels;

		float movementSpeed = 10.f;
		float snapAmount = 1.f;
		float exposure = 1.f;
		float lightThreshold = 0.03;
		float editDistance = 5;
		bool snapEnabled = false;

		TRS cursorBuf;
		TRS transformBuf;

		// Map editing things
		// TODO: don't need dynamic_models anymore
		std::vector<editorEntry> dynamic_models;

		// populates map object tree
		// XXX:  referred to by gameEditorUI
		// TODO: not this
		void addnodes(std::string name,
		              sceneNode::ptr obj,
		              std::set<sceneNode*>& selectedPath);
		void addnodesRec(const std::string& name,
		                 sceneNode::ptr obj,
		                 std::set<sceneNode*>& selectedPath);

		// TODO: why is this in the editor class?
		// list of log messages
		std::list<std::string> logEntries;

		ecs::ref<ecs::entity> getSelectedEntity(void) { return curEntity; };

		void setSelectedEntity(ecs::ref<ecs::entity> ent) {
			this->curEntity = ent;
			this->curNode   = dynamic_ref_cast<sceneNode>(ent);
		}

		sceneNode::ptr getSelectedNode(void) {
			return this->curNode;
		}

	private:
		void updateSelected(const TRS& updated);
		void renderWorldObjects();

		void handleMoveRotate();
		void handleSelectObject();
		void handleCursorUpdate();
		void loadUIModels(void);
		void loadInputBindings();
		void showLoadingScreen();
		bool isUIObject(sceneNode::ptr obj);

		modalSDLInput inputBinds;

		// TODO: utility function, should be move somewhere else
		sceneNode::ptr getNonModel(sceneNode::ptr obj);

		std::vector<editCallback> callbacks;
		std::vector<std::pair<uint32_t, ecs::entity*>> clickState;

		// last place the mouse was clicked, used for determining the amount of
		// rotation, movement etc
		// (position / width, height)
		float clickedX, clickedY;
		// distance from cursor to camera at the last click
		float clickDepth;

		sceneNode::ptr        curNode;
		ecs::ref<ecs::entity> curEntity;
};

class gameEditorUI : public gameView {
	public:
		typedef std::shared_ptr<gameEditorUI> ptr;
		typedef std::weak_ptr<gameEditorUI> weakptr;

		gameEditorUI();

		virtual void handleEvent(const SDL_Event& ev);
		virtual void render(renderFramebuffer::ptr fb);
		virtual void update(float delta);

		gameView::ptr   player;
		gameEditor::ptr editor;
		gameView::ptr   currentView;

		// file pane state
		filePane filepane;

		bool showMapWindow = false;
		bool showObjectEditorWindow = false;
		bool showObjectSelectWindow = false;
		bool showEntityEditorWindow = false;
		bool showEntityListWindow = false;
		bool showAddEntityWindow = false;
		bool showProfilerWindow = false;
		bool showMetricsWindow = false;
		bool showSettingsWindow = false;
		bool showLogWindow = false;
		bool showFilePane = false;

		void menubar();
		void mapWindow();
		void objectEditorWindow();
		void objectSelectWindow();
		void entityListWindow();
		void entityEditorWindow();
		void addEntityWindow();
		void metricsWindow();
		void profilerWindow();
		void settingsWindow();
		void logWindow();

		void renderImguiData();
		void renderEditor();
};

// namespace grendx
}
