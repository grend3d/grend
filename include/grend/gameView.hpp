#pragma once

#include <grend/renderFramebuffer.hpp>
#include <grend/gameMain.hpp>
#include <grend/gameState.hpp>
#include <grend/camera.hpp>
#include <grend/modalSDLInput.hpp>

namespace grendx {

class gameMain;

class gameView {
	public:
		typedef std::shared_ptr<gameView> ptr;
		typedef std::weak_ptr<gameView> weakptr;

		virtual ~gameView();

		virtual void onResume(gameMain *game);
		virtual void onHide(gameMain *game);

		virtual void handleEvent(gameMain *game, const SDL_Event& ev);
		virtual void update(gameMain *game, float delta) = 0;
		virtual void render(gameMain *game, renderFramebuffer::ptr fb) = 0;

		// TODO: game object
		// TODO: this doesn't really need to be part of the view
		camera::ptr cam = std::make_shared<camera>();
		//modalSDLInput input;
};

// namespace grendx
}
