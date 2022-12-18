#pragma once

#include <SDL.h>
//#include <SDL_ttf.h>
#include <grend/openglIncludes.hpp>
#include <grend/renderSettings.hpp>
#include <grend/IoC.hpp>

namespace grendx {

class SDLContext : public IoC::Service {
	public:
		SDLContext(const char *progname, const renderSettings& settings);
		// XXX: context normally shouldn't be copied or moved after initialization,
		//      helps detect some bugs
		SDLContext(const SDLContext& other) = delete;
		SDLContext(const SDLContext&& other) = delete;

		virtual ~SDLContext();
		void setAudioCallback(void *data, SDL_AudioCallback callback);
		void applySettings(const renderSettings& settings);
		const renderSettings& getSettings();

		SDL_Window *window;
		SDL_GLContext glcontext;
		renderSettings currentSettings;

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
