#include <grend/main_logic.hpp>

#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/types.h>

#include <iostream>
#include <memory>

using namespace grendx;

context ctx("grend test");
game_state *gscene = nullptr;

int render_step(double time, void *data) {
	gscene->frame_timer.start();
	gscene->input(ctx);

	if (gscene->running) {
		gscene->physics(ctx);
		gscene->logic(ctx);
		gscene->render(ctx);

		//auto minmax = framems_minmax();
		std::pair<float, float> minmax = {0, 0}; // TODO: implementation
		float fps = gscene->frame_timer.average();

		std::string foo =
			std::to_string(fps) + " FPS "
			+ "(" + std::to_string(1.f/fps * 1000) + "ms/frame) "
			+ "(min: " + std::to_string(minmax.first) + ", "
			+ "max: " + std::to_string(minmax.second) + ")"
			;

		gscene->draw_debug_string(foo);
		SDL_GL_SwapWindow(ctx.window);
		gscene->frame_timer.stop();
	}

	return gscene->running;
}

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#endif

int main(int argc, char *argv[]) {
	std::cerr << "entering main()" << std::endl;
	std::cerr << "started SDL context" << std::endl;
	std::cerr << "have game state" << std::endl;

	gscene = new game_state(ctx);

#ifdef __EMSCRIPTEN__
	emscripten_request_animation_frame_loop(&render_step, nullptr);

#else
	while (gscene->running) {
		render_step(0, nullptr);
	}
#endif

	return 0;
}
