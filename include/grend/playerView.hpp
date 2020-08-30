#pragma once

#include <grend/gameState.hpp>
#include <grend/gameView.hpp>
#include <grend/gameMain.hpp>
#include <grend/modalSDLInput.hpp>
#include <grend/physics.hpp>
#include <memory>

namespace grendx {

class playerView : public gameView {
	public:
		enum modes {
			Move,
		};

		playerView(gameMain *game);
		virtual void handleInput(gameMain *game, SDL_Event& ev);
		virtual void render(gameMain *game);
		void logic(gameMain *game, float delta);

		camera::ptr cam = std::make_shared<camera>();
		gameObject::ptr cameraObj = std::make_shared<gameObject>();
		uint64_t cameraPhysID;
		modalSDLInput input;
};

// namespace grendx
}
