#include <grend/gameMain.hpp>
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

gameMain::gameMain(std::string name)
	: ctx(name.c_str())
{
	initializeOpengl();

#if defined(PHYSICS_BULLET)
	phys   = std::dynamic_pointer_cast<physics>(std::make_shared<bulletPhysics>());
#elif defined(PHYSICS_IMP)
	phys   = std::dynamic_pointer_cast<physics>(std::make_shared<impPhysics>());
#else
#error "No physics implementation defined!"
#endif

	state  = gameState::ptr(new gameState());
	rend   = renderContext::ptr(new renderContext(ctx));
	audio  = audioMixer::ptr(new audioMixer(ctx));
	jobs   = jobQueue::ptr(new jobQueue());
}

void gameMain::clearMetrics(void) {
	metrics.drawnMeshes = 0;
}

int gameMain::step(void) {
	frame_timer.stop();
	frame_timer.start();
	clearMetrics();
	handleInput();

	if (running) {
		// XXX: remove
		Uint32 cur_ticks = SDL_GetTicks();
		Uint32 ticks_delta = cur_ticks - last_frame_time;
		float fticks = ticks_delta / 1000.0f;
		last_frame_time = cur_ticks;

		if (view == nullptr) {
			std::cerr <<
				"ERROR: no view defined, you must set a view controller "
				"with gameMain::setView()"
				<< std::endl;
			running = false;
		}

		view->logic(this, fticks);
		if (jobs != nullptr) {
			jobs->runDeferred();
		}

		setDefaultGlFlags();
		rend->framebuffer->clear();
		view->render(this);

		SDL_GL_SwapWindow(ctx.window);
	}

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
			view->handleInput(this, ev);
		}
	}
}

void grendx::renderWorld(gameMain *game, camera::ptr cam, renderFlags& flags) {
	assert(game->rend && game->rend->framebuffer);

	int winsize_x, winsize_y;
	SDL_GetWindowSize(game->ctx.window, &winsize_x, &winsize_y);

	if (game->state->rootnode) {
		renderQueue que(cam);
		float fticks = SDL_GetTicks() / 1000.0f;

		que.add(game->state->rootnode, fticks);
		que.add(game->state->physObjects);
		que.updateLights(game->rend);
		que.updateReflections(game->rend);
		DO_ERROR_CHECK();

		game->rend->framebuffer->framebuffer->bind();
		DO_ERROR_CHECK();

		// TODO: constants for texture bindings, no magic numbers floating around
		flags.mainShader->bind();
		flags.mainShader->set("time_ms", SDL_GetTicks() * 1.f);
		DO_ERROR_CHECK();

		que.cull(game->rend->framebuffer->width,
		         game->rend->framebuffer->height,
		         game->rend->lightThreshold);
		que.sort();
		buildTilemap(que, game->rend);
		que.updateReflectionProbe(game->rend);
		game->metrics.drawnMeshes +=
			que.flush(game->rend->framebuffer, game->rend, flags);
		game->rend->defaultSkybox.draw(cam, game->rend->framebuffer);
	}
}
