#include <grend-config.h>

#include <grend/gameMain.hpp>
#include <grend/timers.hpp>
#include <grend/scancodes.hpp>

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

using namespace grendx;

// non-pure virtual destructors for rtti
gameMain::~gameMain() {};

gameMain::gameMain(const std::string& name, const renderSettings& _settings)
	: ctx(name.c_str(), _settings),
	  settings(_settings)
{
	initializeOpengl();

#if defined(PHYSICS_BULLET)
	phys   = std::dynamic_pointer_cast<physics>(std::make_shared<bulletPhysics>());
#elif defined(PHYSICS_IMP)
	phys   = std::dynamic_pointer_cast<physics>(std::make_shared<impPhysics>());
#else
#error "No physics implementation defined!"
#endif

	state  = std::make_shared<gameState>();
	rend   = std::make_shared<renderContext>(ctx, _settings);
	audio  = std::make_shared<audioMixer>(ctx);
	jobs   = std::make_shared<jobQueue>();

	entities  = std::make_shared<ecs::entityManager>(this);
	factories = std::make_shared<ecs::serializer>();
}

void gameMain::applySettings(const renderSettings& newSettings) {
	ctx.applySettings(newSettings);
}

void gameMain::clearMetrics(void) {
	metrics.drawnMeshes = 0;
}

int gameMain::step(void) {
	// TODO: kinda disperate set of metrics, should unify/clean this up
	frame_timer.stop();
	frame_timer.start();
	clearMetrics();
	profile::newFrame();
	handleInput();

	if (running) {
		// XXX: remove
		Uint32 cur_ticks = SDL_GetTicks();
		Uint32 ticks_delta = cur_ticks - last_frame_time;
		float fticks = ticks_delta / 1000.0f;
		last_frame_time = cur_ticks;

		updateKeyStates(fticks);

		if (view == nullptr) {
			SDL_Log(
				"ERROR: no view defined, you must set a view controller "
				"with gameMain::setView()"
			);
			running = false;
			return running;
		}

		profile::startGroup("View");
		view->update(this, fticks);
		profile::endGroup();

		profile::startGroup("Syncronous jobs");
		if (jobs != nullptr) {
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
				SDL_Log("Exceeded time limit for deferred job!");
			}

			// left here for debugging, just in case
			//jobs->runDeferred();
		}
		profile::endGroup();

		profile::startGroup("Render");
		setDefaultGlFlags();
		rend->framebuffer->clear();
		view->render(this, rend->framebuffer);

		if (phys) {
			phys->drawDebug(view->cam->viewProjTransform());
		}

		SDL_GL_SwapWindow(ctx.window);
		profile::endGroup();
	}

	profile::endFrame();
	return running;
}

#ifdef __EMSCRIPTEN__
// non-member step function for emscripten
static int render_step(double time, void *data) {
	gameMain *game = static_cast<gameMain*>(data);
	return game->step();
}
#endif

int gameMain::run(void) {
	running = true;

#ifdef __EMSCRIPTEN__
	emscripten_request_animation_frame_loop(&render_step, this);

#else
	while (running) {
		step();
	}
#endif

	return false;
}

void gameMain::setView(std::shared_ptr<gameView> nview) {
	view = nview;

	if (audio != nullptr && view != nullptr && view->cam != nullptr) {
		audio->setCamera(view->cam);
	}
}

void gameMain::handleInput(void) {
	SDL_Event ev;

	while (SDL_PollEvent(&ev)) {
		if (ev.type == SDL_QUIT) {
			running = false;
			return;
		}

		if (view != nullptr) {
			view->handleEvent(this, ev);
		}
	}
}
