#include <grend/gameMain.hpp>

using namespace grendx;

int gameMain::step(void) {
	frame_timer.start();
	handleInput();

	if (running) {
		physics();
		logic();
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

void gameMain::physics(void) {

}

void gameMain::logic(void) {

}
