#pragma once

#include <grend/IoC.hpp>
#include <grend/renderSettings.hpp>
#include <grend/timers.hpp>

// TODO: not this
#include <grend/gameView.hpp>

#include <memory>
#include <stdint.h>

namespace grendx::engine {

void initialize(const std::string& name, const renderSettings& settings);
void step(gameView::ptr view);
void run(gameView::ptr view);
void applySettings(const renderSettings& settings);

IoC::Container& Services();

template <typename T>
T* Resolve(void) {
	return Services().resolve<T>();
}


/*
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
*/

// namespace grendx
}
