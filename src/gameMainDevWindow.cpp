#include <grend/gameMainDevWindow.hpp>

using namespace grendx;

gameMainDevWindow::gameMainDevWindow() : gameMain("grend editor") {
	phys   = physics::ptr(new imp_physics());
	state  = game_state::ptr(new game_state());
	rend   = renderer::ptr(new renderer(ctx));
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
