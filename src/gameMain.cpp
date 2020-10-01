#include <grend/gameMain.hpp>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#endif

using namespace grendx;

int gameMain::step(void) {
	frame_timer.stop();
	frame_timer.start();
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

		step_physics();
		view->logic(this, fticks);

		set_default_gl_flags();
		rend->framebuffer->clear();
		view->render(this);

		//auto minmax = framems_minmax();
		std::pair<float, float> minmax = {0, 0}; // TODO: implementation
		float fps = frame_timer.average();

		/*
		std::string foo =
			std::to_string(fps) + " FPS "
			+ "(" + std::to_string(1.f/fps * 1000) + "ms/frame) "
			+ "(min: " + std::to_string(minmax.first) + ", "
			+ "max: " + std::to_string(minmax.second) + ")"
			;
		std::cout << foo << std::endl;
		*/

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

void gameMain::step_physics(void) {
	if (phys) {
		float dt = 1.0/frame_timer.last();

		if (dt > 0 && !std::isnan(dt) && !std::isinf(dt)) {
			phys->step_simulation(dt);
		}
	}
}

void gameMain::logic(void) {

}

void gameMain::setView(std::shared_ptr<gameView> nview) {
	view = nview;
}

void grendx::renderWorld(gameMain *game, camera::ptr cam) {
	assert(game->rend && game->rend->framebuffer);

	int winsize_x, winsize_y;
	SDL_GetWindowSize(game->ctx.window, &winsize_x, &winsize_y);

	if (game->state->rootnode) {
		renderQueue que(cam);
		float fticks = SDL_GetTicks() / 1000.0f;

		que.add(game->state->rootnode, fticks);
		que.updateLights(game->rend->shaders["shadow"], game->rend->atlases);
		que.updateReflections(game->rend->shaders["refprobe"],
		                      game->rend->atlases,
		                      game->rend->defaultSkybox);
		que.add(game->state->physObjects);
		DO_ERROR_CHECK();

		game->rend->framebuffer->framebuffer->bind();
		DO_ERROR_CHECK();

		// TODO: constants for texture bindings, no magic numbers floating around
		game->rend->shaders["main"]->bind();
		glActiveTexture(GL_TEXTURE6);
		game->rend->atlases.reflections->color_tex->bind();
		game->rend->shaders["main"]->set("reflection_atlas", 6);
		glActiveTexture(GL_TEXTURE7);
		game->rend->atlases.shadows->depth_tex->bind();
		game->rend->shaders["main"]->set("shadowmap_atlas", 7);
		DO_ERROR_CHECK();

		game->rend->shaders["main"]->set("time_ms", SDL_GetTicks() * 1.f);
		DO_ERROR_CHECK();

		auto flags = game->rend->getFlags();
		que.flush(game->rend->framebuffer, flags, game->rend->atlases);
		game->rend->defaultSkybox.draw(cam, game->rend->framebuffer);
	}
}
