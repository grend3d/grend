#pragma once

#include <grend/gameState.hpp> // TODO: rename to gameState.h
#include <grend/engine.hpp>     // TODO: rename to renderer.h
#include <grend/glManager.hpp> // TODO: rename
#include <grend/gameView.hpp>
#include <grend/jobQueue.hpp>
#include <grend/timers.hpp>
#include <grend/audioMixer.hpp>

#include <grend/ecs/ecs.hpp>
#include <grend/ecs/serializer.hpp>

#include <memory>
#include <stdint.h>

namespace grendx {

// gameView takes gameMain pointers
class gameView;

// abstract class, should be derived from
class gameMain {
	public:
		gameMain(const std::string& name, const renderSettings& settings);
		virtual ~gameMain();

		virtual int step(void);
		virtual int run(void);

		virtual void setView(std::shared_ptr<gameView> nview);
		virtual void handleInput(void);
		virtual void clearMetrics(void);
		void applySettings(const renderSettings& newSettings);

		bool running = false;
		context ctx;
		renderSettings settings;

		// state objects
		// TODO: these might as well be unique_ptrs
		std::shared_ptr<gameState>     state = nullptr;
		std::shared_ptr<gameView>      view  = nullptr;
		std::shared_ptr<renderContext> rend  = nullptr;
		std::shared_ptr<physics>       phys  = nullptr;
		std::shared_ptr<audioMixer>    audio = nullptr;
		std::shared_ptr<jobQueue>      jobs  = nullptr;
		// ECS state
		std::shared_ptr<ecs::entityManager> entities    = nullptr;
		std::shared_ptr<ecs::serializer>    factories = nullptr;

		// FPS info
		sma_counter frame_timer;
		uint32_t last_frame_time = 0;

		struct {
			unsigned drawnMeshes = 0;
		} metrics;
};

// namespace grendx
}
