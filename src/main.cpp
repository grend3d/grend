#include <grend/main_logic.hpp>

#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/types.h>

#include <iostream>
#include <memory>

using namespace grendx;

int main(int argc, char *argv[]) {
	context ctx("grend test");
	std::unique_ptr<testscene> scene(new testscene(ctx));

	while (scene->running) {
		//uint32_t begin = SDL_GetTicks();
		scene->frame_timer.start();
		scene->input(ctx);

		if (scene->running) {
			scene->logic(ctx);
			scene->render(ctx);

			//auto minmax = framems_minmax();
			std::pair<float, float> minmax = {0, 0}; // TODO: implementation
			float fps = scene->frame_timer.average();

			std::string foo =
				std::to_string(fps) + " FPS "
				+ "(" + std::to_string(1.f/fps * 1000) + "ms/frame) "
				+ "(min: " + std::to_string(minmax.first) + ", "
				+ "max: " + std::to_string(minmax.second) + ")"
				;

			scene->draw_debug_string(foo);
			SDL_GL_SwapWindow(ctx.window);
			scene->frame_timer.stop();

			/*
			uint32_t end = SDL_GetTicks() - begin;
			fps = fps_sma(end);
			*/

			/*
			// TODO: without vsync
			double frametime = 1.f/fps*1000;

			if (end < scene->dsr_target_ms) {
				SDL_Delay(floor(scene->dsr_target_ms - end));
			}
			*/

			//fflush(stdout);
		}
	}

	return 0;
}
