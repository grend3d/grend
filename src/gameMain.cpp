#include <grend-config.h>

#include <grend/gameMain.hpp>
#include <grend/timers.hpp>
#include <grend/scancodes.hpp>
#include <grend/logger.hpp>

#include <grend/gameState.hpp>
#include <grend/engine.hpp>    // TODO: rename to renderer.h
#include <grend/glManager.hpp>
#include <grend/gameView.hpp>
#include <grend/jobQueue.hpp>
#include <grend/audioMixer.hpp>

#include <grend/ecs/ecs.hpp>
#include <grend/ecs/serializer.hpp>
#include <grend/ecs/serializeDefs.hpp>

#include <string.h> // memset

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#endif

#if defined(PHYSICS_BULLET)
#include <grend/bulletPhysics.hpp>
#elif defined(PHYSICS_IMP)
#include <grend/impPhysics.hpp>
#endif

#include <grend/thumbnails.hpp>

using namespace grendx;
using namespace grendx::engine;

// main IoC container
static IoC::Container gameServices;

static bool running = true;
static uint32_t last_frame_time = 0;

IoC::Container& grendx::engine::Services() { return gameServices; }

void grendx::engine::initialize(const std::string& name, const renderSettings& settings) {
	Services().bind<SDLContext, SDLContext>(name.c_str(), settings);
	initializeOpengl();

#if defined(PHYSICS_BULLET)
	Services().bind<physics, bulletPhysics>();
#elif defined(PHYSICS_IMP)
	Services().bind<physics, impPhysics>();
#else
#error "No physics implementation defined!"
#endif

	auto& ctx = *Resolve<SDLContext>();
	Services().bind<ecs::entityManager, ecs::entityManager>();
	Services().bind<ecs::serializer,    ecs::serializer>();
	Services().bind<renderContext,      renderContext>(ctx, settings);
	Services().bind<gameState,          gameState>();
	Services().bind<audioMixer,         audioMixer>(&ctx);
	Services().bind<jobQueue,           jobQueue>();
	Services().bind<thumbnails,         thumbnails>();

	ecs::addDefaultFactories();

	LogInfo("gameMain() finished");
}

void handleInput(gameView::ptr view) {
	SDL_Event ev;

	while (SDL_PollEvent(&ev)) {
		if (ev.type == SDL_QUIT) {
			running = false;
			return;
		}

		if (view != nullptr) {
			view->handleEvent(ev);
		}
	}
}

void grendx::engine::step(gameView::ptr view) {
	// TODO: kinda disperate set of metrics, should unify/clean this up
	//frame_timer.stop();
	//frame_timer.start();
	//clearMetrics();
	profile::newFrame();
	handleInput(view);

	auto jobs = Resolve<jobQueue>();
	auto rend = Resolve<renderContext>();
	auto phys = Resolve<physics>();
	auto ctx  = Resolve<SDLContext>();

	if (running) {
		// XXX: remove
		Uint32 cur_ticks = SDL_GetTicks();
		Uint32 ticks_delta = cur_ticks - last_frame_time;
		float fticks = ticks_delta / 1000.0f;
		last_frame_time = cur_ticks;

		updateKeyStates(fticks);

		if (view == nullptr) {
			LogError(
				"ERROR: no view defined, you must set a view controller "
				"with gameMain::setView()"
			);
			running = false;
			return;
			//return running;
		}

		profile::startGroup("View");
		view->update(fticks);
		profile::endGroup();

		profile::startGroup("Syncronous jobs");

		{
			// spread long-running syncronous job batches across multiple frames
			// TODO: more fine-grained than SDL_GetTicks()
			//       could use std::chrono high res clock
			uint32_t start = SDL_GetTicks();
			uint32_t ticks = start;
			// TODO: configurable max time
			uint32_t maxTime = 2 /*ms*/;

			bool running = true;

			while (running && ticks - start < maxTime) {
				running = jobs->runSingleDeferred();
				ticks = SDL_GetTicks();
			}

			if (ticks - start >= maxTime) {
				LogError("Exceeded time limit for deferred job!");
			}

			// left here for debugging, just in case
			//jobs->runDeferred();
		}
		profile::endGroup();

		profile::startGroup("Render");
		setDefaultGlFlags();
		rend->framebuffer->clear();
		view->render(rend->framebuffer);

		SDL_GL_SwapWindow(ctx->window);
		profile::endGroup();
	}

	profile::endFrame();
	//return running;
}

#ifdef __EMSCRIPTEN__
// non-member step function for emscripten
static int render_step(double time, void *data, gameView::ptr view) {
	gameMain *game = static_cast<gameMain*>(data);
	return game->step(view);
}
#endif

void grendx::engine::run(gameView::ptr view) {
	running = true;

#ifdef __EMSCRIPTEN__
	emscripten_request_animation_frame_loop(&render_step, this, view);

#else
	while (running) {
		step(view);
	}
#endif
}
