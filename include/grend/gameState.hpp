#pragma once

#include <grend/gameObject.hpp>
#include <grend/gameModel.hpp>
#include <grend/glmIncludes.hpp>
#include <grend/geometryGeneration.hpp>
#include <grend/glManager.hpp>
#include <grend/utility.hpp>
#include <grend/text.hpp>
#include <grend/physics.hpp>
#include <grend/camera.hpp>

#include <tuple>
#include <vector>
#include <fstream>
#include <iostream>
#include <sstream>
#include <iterator>
#include <string>
#include <exception>
#include <memory>
#include <utility>
#include <map>
#include <list>

namespace grendx {

class resolutionScale {
	public:
		// dynamic resolution scaling
		void adjust_draw_resolution(void);
		float dsrTargetFps = 60.0;
		float dsrTargetMs = 1000/dsrTargetFps;
		float dsrScaleX = 1.0;
		float dsrScaleY = 1.0;
		/*
		// TODO: use this once I figure out how to do vsync
		float dsrScaleDown = dsrTargetMs * 0.90;
		float dsrScaleUp = dsrTargetMs * 0.80;
		*/
		float dsrScaleDown = dsrTargetMs * 1.1;
		float dsrScaleUp = dsrTargetMs * 1.03;
		float dsrMinScaleX = 0.50;
		float dsrMinScaleY = 0.50;
		float dsrDownIncr = 0.10;
		float dsrUpIncr = 0.01;

};

// TODO: rename to gameState
class gameState {
	public:
		typedef std::shared_ptr<gameState> ptr;
		typedef std::weak_ptr<gameState> weakptr;
		
		gameState();
		virtual ~gameState();

		gameObject::ptr rootnode;
		gameObject::ptr physObjects;
		// XXX: 
		modelMap loadedModels;

	private:

};

// namespace grendx;
}
