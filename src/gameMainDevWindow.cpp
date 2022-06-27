#include <grend-config.h>

#include <grend/gameMainDevWindow.hpp>
#include <grend/jobQueue.hpp>
#include <grend/audioMixer.hpp>

#ifdef PHYSICS_BULLET
#include <grend/bulletPhysics.hpp>
#elif defined(PHYSICS_IMP)
// TODO: the great camelCasification
#include <grend/impPhysics.hpp>
#endif

using namespace grendx;

gameMainDevWindow::gameMainDevWindow(const renderSettings& settings)
	: gameMain("grend editor", settings)
{
	//editor = gameView::ptr(new gameEditor(this));
	editor = std::make_shared<gameEditor>(this);
	view   = editor;

	auto jobs  = services.resolve<jobQueue>();
	auto audio = services.resolve<audioMixer>();

	jobs->addAsync([](){
		std::cout << "job queue seems to be working!" << std::endl;
		return true;
	});

	audio->setCamera(view->cam);

	/*
	input.bind(MODAL_ALL_MODES,
		[&, this] (SDL_Event& ev, unsigned flags) {
			if (ev.type == SDL_KEYDOWN && ev.key.keysym.sym == SDLK_b) {
				std::cout << "adding an async task..." << std::endl;
				jobs->addAsync([=] () {
					unsigned long sum = 0;
					unsigned target = rand();
					for (unsigned i = 0; i < target; i++) sum += i;

					std::cout
						<< "hey, asyncronous task here! launched from a lambda!"
						<< std::endl;
					std::cout << "sum " << target << ": " << sum << std::endl;

					auto fut = jobs->addDeferred([=] () {
						std::cout
							<< "sup, deferred task here. very cool. "
							<< "(sum was " << sum << " btw)"
							<< std::endl;
						return true;
					});

					std::cout << "waiting for deferred call..." << std::endl;
					fut.wait();
					std::cout << "have deferred: " << fut.get() << std::endl;

					return true;
				});
			}

			return MODAL_NO_CHANGE;
		});

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
	*/
}

void gameMainDevWindow::setView(std::shared_ptr<gameView> nview) {
	player = nview;
}

void gameMainDevWindow::handleInput(void) {
	auto audio = services.resolve<audioMixer>();

	SDL_Event ev;
	while (SDL_PollEvent(&ev)) {
		if (ev.type == SDL_QUIT) {
			running = false;
			return;
		}

		if (mode == modes::Editor
			&& ev.type == SDL_KEYDOWN
			&& ev.key.keysym.sym == SDLK_F1
			/*&& (flags & bindFlags::Control)*/)
		{
			view = player;
			audio->setCamera(view->cam);

			if (audio->currentCam == nullptr) {
				std::cerr
					<< "no camera is defined in the audio mixer...?"
					<< std::endl;
			}

			mode = modes::Player;
		}

		else if (mode == modes::Player
			&& ev.type == SDL_KEYDOWN
			&& ev.key.keysym.sym == SDLK_F1
			/*&& (flags & bindFlags::Control)*/)
		{
			view = editor;
			audio->setCamera(view->cam);
			mode = modes::Editor;
		}

		view->handleEvent(this, ev);
	}
}

void gameMainDevWindow::addEditorCallback(gameEditor::editCallback func) {
	gameEditor *p = editor.get();
	p->addEditorCallback(func);
}
