#pragma once

#include <SDL.h>
//#include <SDL_ttf.h>
#include <grend/openglIncludes.hpp>
#include <grend/renderSettings.hpp>

namespace grendx {

class context {
	public:
		context(const char *progname, const renderSettings& settings);
		// XXX: context normally shouldn't be copied or moved after initialization,
		//      helps detect some bugs
		context(const context& other) = delete;
		context(const context&& other) = delete;

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
