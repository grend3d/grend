#include <grend/gameMain.hpp>

using namespace grendx;

int gameMain::step(void) {
	return running;
}

int gameMain::run(void) {
	running = true;

	while (running) {
		step();
	}

	return false;
}
