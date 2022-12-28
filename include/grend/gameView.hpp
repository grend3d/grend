#pragma once

#include <grend/renderFramebuffer.hpp>
#include <grend/camera.hpp>
#include <grend/modalSDLInput.hpp>

namespace grendx {

class gameView {
	public:
		typedef std::shared_ptr<gameView> ptr;
		typedef std::weak_ptr<gameView> weakptr;

		virtual ~gameView();

		virtual void onResume();
		virtual void onHide();

		virtual void handleEvent(const SDL_Event& ev);
		virtual void update(float delta) = 0;
		virtual void render(renderFramebuffer::ptr fb) = 0;

		// TODO: game object
		// TODO: this doesn't really need to be part of the view
		camera::ptr cam = std::make_shared<camera>();
		//modalSDLInput input;
};

// namespace grendx
}
