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

	auto audio = services.resolve<audioMixer>();
	audio->setCamera(view->cam);
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
				LogWarn("no camera is defined in the audio mixer...?");
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
