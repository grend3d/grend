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
		typedef std::shared_ptr<playerView> ptr;
		typedef std::weak_ptr<playerView>   weakptr;

		playerView();
		virtual void render();
		virtual void logic(float delta);

		/*
	private:
		void drawMainMenu(int wx, int wy);
		*/
};

// namespace grendx
}
