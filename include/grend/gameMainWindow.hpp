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
class gameMainWindow : public gameMain {
	public:
		enum modes {
			Editor,
			Player,
		};

		gameMainWindow();
		virtual void handleInput(void);
};

// namespace grendx
}
