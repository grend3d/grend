#include <grend/gameMainDevWindow.hpp>

using namespace grendx;

gameMainDevWindow::gameMainDevWindow() : gameMain("[grend editor]") {
	phys   = physics::ptr(new imp_physics());
	state  = game_state::ptr(new game_state());
	rend   = renderer::ptr(new renderer(ctx));

	player = gameView::ptr(new playerView(this));
	editor = gameView::ptr(new game_editor(this));
	view   = editor;

	input.bind(modes::Editor,
		[&] (SDL_Event& ev, unsigned flags) {
			if (ev.type == SDL_KEYDOWN
			    && ev.key.keysym.sym == SDLK_m
			    && (flags & bindFlags::Control))
			{
				view = player;
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
				return (int)modes::Editor;
			}
			return MODAL_NO_CHANGE;
		});

	input.setMode(modes::Editor);
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
