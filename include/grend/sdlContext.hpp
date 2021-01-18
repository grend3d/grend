#pragma once

// premium HD resolution
// TODO: make screen size configurable
/*
#define SCREEN_SIZE_X 1366
#define SCREEN_SIZE_Y 768
*/
#define SCREEN_SIZE_X 1280
#define SCREEN_SIZE_Y 720

#include <SDL.h>
//#include <SDL_ttf.h>
#include <grend/openglIncludes.hpp>

namespace grendx {

class context {
	public:
		context(const char *progname);
		~context();
		void setAudioCallback(void *data, SDL_AudioCallback callback);

		SDL_Window *window;
		SDL_GLContext glcontext;

		SDL_AudioDeviceID audioOut;
		SDL_AudioSpec     audioHave;
		SDL_AudioCallback audioCallback = nullptr;
		void *callbackData = nullptr;

		GLenum glew_status;
};

// throws
void SDL_Die(const char *message);

// namespace grendx
}
