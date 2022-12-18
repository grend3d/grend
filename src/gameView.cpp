#include <grend/gameView.hpp>

using namespace grendx;

// non-pure virtual destructors for rtti
gameView::~gameView() {};

void gameView::handleEvent(const SDL_Event& ev) {}
void gameView::onResume() {}
void gameView::onHide() {}
