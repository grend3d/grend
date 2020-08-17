#pragma once

#include <grend/gameState.hpp>
#include <grend/gameView.hpp>
#include <grend/gameMain.hpp>

namespace grendx {

class playerView : public gameView {
	public:
		playerView() : gameView() {};
		virtual void handleInput(gameMain *game, SDL_Event& ev);
		virtual void render(gameMain *game);
};

// namespace grendx
}
