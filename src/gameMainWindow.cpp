#include <grend/gameMainWindow.hpp>

using namespace grendx;

#if defined(PHYSICS_BULLET)
#include <grend/bulletPhysics.hpp>
#elif defined(PHYSICS_IMP)
// TODO: the great camelCasification
#include <grend/impPhysics.hpp>
#endif


gameMainWindow::gameMainWindow() : gameMain("grend") {
#if defined(PHYSICS_BULLET)
	phys = physics::ptr(new bulletPhysics());
#elif defined(PHYSICS_IMP)
	phys = physics::ptr(new impPhysics());
#else
#error "No physics implementation defined!"
#endif

	state  = game_state::ptr(new game_state());
	rend   = renderContext::ptr(new renderContext(ctx));
	audio  = audioMixer::ptr(new audioMixer(ctx));
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
