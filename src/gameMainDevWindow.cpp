#include <grend/gameMainDevWindow.hpp>

#ifdef PHYSICS_BULLET
#include <grend/bulletPhysics.hpp>
#elif defined(PHYSICS_IMP)
// TODO: the great camelCasification
#include <grend/imp_physics.hpp>
#endif

using namespace grendx;

gameMainDevWindow::gameMainDevWindow() : gameMain("grend editor") {
#if defined(PHYSICS_BULLET)
	phys   = std::dynamic_pointer_cast<physics>(std::make_shared<bulletPhysics>());
#elif defined(PHYSICS_IMP)
	phys   = std::dynamic_pointer_cast<physics>(std::make_shared<impPhysics>());
#else
#error "No physics implementation defined!"
#endif

	state  = game_state::ptr(new game_state());
	rend   = renderContext::ptr(new renderContext(ctx));
	audio  = audioMixer::ptr(new audioMixer(ctx));

	editor = gameView::ptr(new game_editor(this));
	view   = editor;
	audio->setCamera(view->cam);

	input.bind(modes::Editor,
		[&] (SDL_Event& ev, unsigned flags) {
			if (ev.type == SDL_KEYDOWN
			    && ev.key.keysym.sym == SDLK_m
			    && (flags & bindFlags::Control))
			{
				view = player;
				audio->setCamera(view->cam);

				if (audio->currentCam == nullptr) {
					std::cerr
						<< "no camera is defined in the audio mixer...?"
						<< std::endl;
				}
				return (int)modes::Player;
			}
			return MODAL_NO_CHANGE;
		});

	input.bind(modes::Player,
		[&] (SDL_Event& ev, unsigned flags) {
			if (ev.type == SDL_KEYDOWN
			    && ev.key.keysym.sym == SDLK_m
			    && (flags & bindFlags::Control))
			{
				view = editor;
				audio->setCamera(view->cam);
				return (int)modes::Editor;
			}
			return MODAL_NO_CHANGE;
		});

	input.setMode(modes::Editor);
}

void gameMainDevWindow::setView(std::shared_ptr<gameView> nview) {
	player = nview;
}

void gameMainDevWindow::handleInput(void) {
	SDL_Event ev;

	while (SDL_PollEvent(&ev)) {
		if (ev.type == SDL_QUIT) {
			running = false;
			return;
		}

		input.dispatch(ev);
		view->handleInput(this, ev);
	}
}
