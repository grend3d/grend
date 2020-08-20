#pragma once
#include <grend/gameMain.hpp>
#include <grend/gameState.hpp>
#include <grend/camera.hpp>

namespace grendx {

class gameMain;

class gameView {
	public:
		typedef std::shared_ptr<gameView> ptr;
		typedef std::weak_ptr<gameView> weakptr;

		virtual void handleInput(gameMain *game, SDL_Event& ev) = 0;
		virtual void logic(gameMain *game, float delta) {};
		virtual void render(gameMain *game) = 0;
		// TODO: game object
		camera cam;
};

// namespace grendx
}
