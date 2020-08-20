#pragma once

#include <grend/gameState.hpp> // TODO: rename to gameState.h
#include <grend/engine.hpp>     // TODO: rename to renderer.h
#include <grend/gl_manager.hpp> // TODO: rename
#include <grend/gameView.hpp>
#include <grend/timers.hpp>
#include <memory>
#include <stdint.h>

namespace grendx {

// gameView takes gameMain pointers
class gameView;

// abstract class, should be derived from
class gameMain {
	public:
		gameMain(std::string name="[grendx]") : ctx(name.c_str()) {
			initialize_opengl();
		}
		virtual int step(void);
		virtual int run(void);
		virtual void physics(void);
		virtual void logic(void);
		virtual void handleInput(void) = 0;

		bool running = false;
		context ctx;
		std::shared_ptr<game_state> state = nullptr;
		std::shared_ptr<gameView>   view  = nullptr;
		std::shared_ptr<renderer>   rend  = nullptr;

		// FPS info
		sma_counter frame_timer;
		uint32_t last_frame_time = 0;
};

// namespace grendx
}
