#include <grend/gameView.hpp>

using namespace grendx;

// non-pure virtual destructors for rtti
gameView::~gameView() {};

void gameView::handleInput(gameMain *game, SDL_Event& ev) {
	input.dispatch(ev);
}
