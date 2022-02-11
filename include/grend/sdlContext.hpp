#pragma once

#include <SDL.h>
//#include <SDL_ttf.h>
#include <grend/openglIncludes.hpp>
#include <grend/renderSettings.hpp>

namespace grendx {

class context {
	public:
		context(const char *progname, const renderSettings& settings);
		~context();
		void setAudioCallback(void *data, SDL_AudioCallback callback);
		void applySettings(const renderSettings& settings);

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
