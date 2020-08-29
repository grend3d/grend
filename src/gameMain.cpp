#include <grend/gameMain.hpp>

using namespace grendx;

int gameMain::step(void) {
	frame_timer.start();
	handleInput();

	if (running) {
		Uint32 cur_ticks = SDL_GetTicks();
		Uint32 ticks_delta = cur_ticks - last_frame_time;
		float fticks = ticks_delta / 1000.0f;
		last_frame_time = cur_ticks;

		step_physics();
		view->logic(this, fticks);
		view->render(this);

		//auto minmax = framems_minmax();
		std::pair<float, float> minmax = {0, 0}; // TODO: implementation
		float fps = frame_timer.average();

		std::string foo =
			std::to_string(fps) + " FPS "
			+ "(" + std::to_string(1.f/fps * 1000) + "ms/frame) "
			+ "(min: " + std::to_string(minmax.first) + ", "
			+ "max: " + std::to_string(minmax.second) + ")"
			;

		SDL_GL_SwapWindow(ctx.window);
		frame_timer.stop();
	}

	return running;
}

int gameMain::run(void) {
	running = true;

	while (running) {
		step();
	}

	return false;
}

void gameMain::step_physics(void) {
	if (phys) {
		float dt = 1.0/frame_timer.average();

		if (dt > 0 && !std::isnan(dt) && !std::isinf(dt)) {
			phys->step_simulation(dt);
		}
	}
}

void gameMain::logic(void) {

}
