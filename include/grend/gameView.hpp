#pragma once
#include <grend/gameMain.hpp>
#include <grend/gameState.hpp>
#include <grend/camera.hpp>
#include <grend/modalSDLInput.hpp>

namespace grendx {

class gameMain;

class gameView {
	public:
		typedef std::shared_ptr<gameView> ptr;
		typedef std::weak_ptr<gameView> weakptr;

		virtual ~gameView();

		virtual void handleInput(gameMain *game, SDL_Event& ev);
		virtual void logic(gameMain *game, float delta) = 0;
		virtual void render(gameMain *game) = 0;

		// TODO: game object
		camera::ptr cam = std::make_shared<camera>();
		modalSDLInput input;
};

// namespace grendx
}
