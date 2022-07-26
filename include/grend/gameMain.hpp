#pragma once

#include <grend/IoC.hpp>
#include <grend/renderSettings.hpp>
#include <grend/sdlContext.hpp>
#include <grend/timers.hpp>

#include <memory>
#include <stdint.h>

namespace grendx {

class gameView;

// abstract class, should be derived from
class gameMain {
	public:
		// TODO: remove render settings argument from here, seperate
		//       renderer/services initialization from window creation
		gameMain(const std::string& name, const renderSettings& settings);
		virtual ~gameMain();

		virtual int step(void);
		virtual int run(void);

		virtual void setView(std::shared_ptr<gameView> nview);
		virtual void handleInput(void);
		virtual void clearMetrics(void);
		void applySettings(const renderSettings& newSettings);

		bool running = false;
		IoC::Container services;
		// TODO: maybe make SDL context a "window" service that gets bound
		context ctx;
		renderSettings settings;

		std::shared_ptr<gameView> view  = nullptr;

		// FPS info
		sma_counter frame_timer;
		uint32_t last_frame_time = 0;

		struct {
			unsigned drawnMeshes = 0;
		} metrics;
};

// namespace grendx
}

// TODO: maybe not part of gameMain
#include <grend/gameView.hpp>
