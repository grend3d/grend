#pragma once

#include <grend/gameState.hpp>   // TODO: rename to gameState.h
#include <grend/engine.hpp>      // TODO: rename to renderer.h
#include <grend/gameEditor.hpp> // TODO: gameEditor
#include <grend/impPhysics.hpp>
#include <grend/gameMain.hpp>
#include <grend/gameView.hpp>
#include <grend/timers.hpp>
#include <grend/modalSDLInput.hpp>
#include <memory>

namespace grendx {

// development instance with editor, a production instance would only
// need a player view
class gameMainDevWindow : public gameMain {
	public:
		enum modes {
			Editor,
			Player,
		};

		gameMainDevWindow();

		// setView here sets the player view
		virtual void setView(std::shared_ptr<gameView> nview);
		virtual void handleInput(void);

		gameView::ptr player = nullptr;
		gameView::ptr editor = nullptr;
		modalSDLInput input;
};

// namespace grendx
}
