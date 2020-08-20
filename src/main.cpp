#include <grend/gameMainDevWindow.hpp>

#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/types.h>

#include <iostream>
#include <memory>

using namespace grendx;

gameMain *game;

int render_step(double time, void *data) {
	return game->step();
}

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#endif

int main(int argc, char *argv[]) {
	std::cerr << "entering main()" << std::endl;
	std::cerr << "started SDL context" << std::endl;
	std::cerr << "have game state" << std::endl;

	game = new gameMainDevWindow();

#ifdef __EMSCRIPTEN__
	emscripten_request_animation_frame_loop(&render_step, nullptr);

#else
	game->run();
#endif

	return 0;
}
