#pragma once

#include <grend/gameObject.hpp>
#include <grend/gameModel.hpp>
#include <grend/glm-includes.hpp>
#include <grend/geometry-gen.hpp>
#include <grend/gl_manager.hpp>
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
		float dsr_target_fps = 60.0;
		float dsr_target_ms = 1000/dsr_target_fps;
		float dsr_scale_x = 1.0;
		float dsr_scale_y = 1.0;
		/*
		// TODO: use this once I figure out how to do vsync
		float dsr_scale_down = dsr_target_ms * 0.90;
		float dsr_scale_up = dsr_target_ms * 0.80;
		*/
		float dsr_scale_down = dsr_target_ms * 1.1;
		float dsr_scale_up = dsr_target_ms * 1.03;
		float dsr_min_scale_x = 0.50;
		float dsr_min_scale_y = 0.50;
		float dsr_down_incr = 0.10;
		float dsr_up_incr = 0.01;

};

// TODO: rename to gameState
class game_state {
	public:
		typedef std::shared_ptr<game_state> ptr;
		typedef std::weak_ptr<game_state> weakptr;
		
		game_state();
		virtual ~game_state();

		gameObject::ptr rootnode;
		gameObject::ptr physObjects;
		// XXX: 
		model_map loadedModels;

	private:

};

// namespace grendx;
}
