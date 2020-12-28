#include <grend/gameMainWindow.hpp>

using namespace grendx;

#if defined(PHYSICS_BULLET)
#include <grend/bulletPhysics.hpp>
#elif defined(PHYSICS_IMP)
// TODO: the great camelCasification
#include <grend/impPhysics.hpp>
#endif


gameMainWindow::gameMainWindow() : gameMain("grend") {

}

void gameMainWindow::handleInput(void) {
	SDL_Event ev;

	while (SDL_PollEvent(&ev)) {
		if (ev.type == SDL_QUIT) {
			running = false;
			return;
		}

		view->handleInput(this, ev);
	}
}
