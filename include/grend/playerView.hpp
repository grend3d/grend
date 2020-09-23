#pragma once

#include <grend/gameState.hpp>
#include <grend/gameView.hpp>
#include <grend/gameMain.hpp>
#include <grend/modalSDLInput.hpp>
#include <grend/physics.hpp>
#include <nanovg/src/nanovg.h>
#include <memory>

namespace grendx {

class playerView : public gameView {
	public:
		typedef std::shared_ptr<playerView> ptr;
		typedef std::weak_ptr<playerView>   weakptr;
		enum modes {
			MainMenu,
			Move,
			Pause,
		};

		playerView(gameMain *game);
		virtual void handleInput(gameMain *game, SDL_Event& ev);
		virtual void render(gameMain *game);
		void logic(gameMain *game, float delta);

		gameObject::ptr cameraObj = std::make_shared<gameObject>();
		uint64_t cameraPhysID;
		modalSDLInput input;
		NVGcontext *vg = nullptr;
		int menuSelect = 0;

	private:
		void drawMainMenu(int wx, int wy);
};

// namespace grendx
}
