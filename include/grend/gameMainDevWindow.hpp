#pragma once

#include <grend/gameState.hpp>   // TODO: rename to gameState.h
#include <grend/engine.hpp>      // TODO: rename to renderer.h
#include <grend/gameEditor.hpp> // TODO: gameEditor
#include <grend/impPhysics.hpp>
#include <grend/gameMain.hpp>
#include <grend/gameView.hpp>
#include <grend/timers.hpp>
#include <grend/modalSDLInput.hpp>
#include <grend/renderSettings.hpp>

#include <memory>
#include <functional>

namespace grendx::engine::dev {

void initialize(const std::string& name, const renderSettings& settings);
void step(gameView::ptr view);
void run(gameView::ptr view);

/*
// development instance with editor, a production instance would only
// need a player view
class gameMainDevWindow : public gameMain {
	public:
		enum modes {
			Editor,
			Player,
		};
		int mode = modes::Editor;

		gameMainDevWindow(const renderSettings& settings);

		// setView here sets the player view
		virtual void setView(gameView::ptr nview);
		virtual void handleInput(void);
		void addEditorCallback(gameEditor::editCallback func);

		gameView::ptr   player = nullptr;
		gameEditor::ptr editor = nullptr;
};
*/

// namespace grendx
}
