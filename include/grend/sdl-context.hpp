#pragma once

// premium HD resolution
// TODO: make screen size configurable
/*
#define SCREEN_SIZE_X 1366
#define SCREEN_SIZE_Y 768
*/
#define SCREEN_SIZE_X 1280
#define SCREEN_SIZE_Y 720

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <grend/opengl-includes.hpp>

namespace grendx {

class context {
	public:
		context(const char *progname);
		~context();

		SDL_Window *window;
		SDL_GLContext glcontext;
		GLenum glew_status;
};

// throws
void SDL_Die(const char *message);

// namespace grendx
}
