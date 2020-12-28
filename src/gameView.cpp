#include <grend/gameView.hpp>

using namespace grendx;

void gameView::handleInput(gameMain *game, SDL_Event& ev) {
	input.dispatch(ev);
}

