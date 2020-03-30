#include <grend/sdl-context.hpp>
#include <stdexcept>

namespace grendx {

void SDL_Die(const char *message) {
	SDL_Quit();
	throw std::logic_error(message);
}

context::context(const char *progname) {
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		SDL_Die("Couldn't initialize video.");
	}

	if (IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG) != (IMG_INIT_JPG | IMG_INIT_PNG)) {
		SDL_Die("Couldn't initialize sdl2_image");
	}

	if (TTF_Init() < 0) {
		SDL_Die("Couldn't initialize sdl2_ttf");
	}

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);

	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

	/*
	// TODO: do I need this for render FB multisampling?
	//       disabling for now
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);
	*/

	window = SDL_CreateWindow(progname, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
	                          SCREEN_SIZE_X, SCREEN_SIZE_Y,
	                          SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);

	SDL_ShowCursor(SDL_DISABLE);

	if (!window) {
		SDL_Die("Couldn't create a window");
	}

	glcontext = SDL_GL_CreateContext(window);
	SDL_GL_SetSwapInterval(1);

	if ((glew_status = glewInit()) != GLEW_OK) {
		SDL_Die("glewInit()");
	}
}

context::~context() {
	SDL_GL_DeleteContext(glcontext);
	SDL_DestroyWindow(window);
	IMG_Quit();
	SDL_Quit();
}

// namespace grendx
}
