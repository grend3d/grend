#pragma once

#include <grend/gameState.hpp> // TODO: rename to gameState.h
#include <grend/engine.hpp>     // TODO: rename to renderer.h
#include <grend/gameMain.hpp>
#include <grend/gameView.hpp>
#include <grend/playerView.hpp>
#include <grend/timers.hpp>
#include <memory>

namespace grendx {

// development instance with editor, a production instance would only
// need a player view
class gameMainDevWindow : public gameMain {
	public:
		gameMainDevWindow() : gameMain("[grend editor]") {
			player = gameView::ptr(new playerView());
			/*
			editor = gameView::ptr(new editorView());
			view   = editor;
			*/
		}
		virtual void handleInput(void);

		std::shared_ptr<gameView> player;
		std::shared_ptr<gameView> editor;

		// FPS info
		sma_counter frame_timer;
};

// namespace grendx
}
