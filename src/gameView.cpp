#include <grend/gameView.hpp>

using namespace grendx;

// non-pure virtual destructors for rtti
gameView::~gameView() {};

void gameView::handleEvent(gameMain *game, const SDL_Event& ev) {}
void gameView::onResume(gameMain *game) {}
void gameView::onHide(gameMain *game) {}
