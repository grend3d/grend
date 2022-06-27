#include <grend/sdlContext.hpp>
#include <stdexcept>
#include <iostream>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#endif

namespace grendx {

void SDL_Die(const char *message) {
	SDL_Log("SDL_Die(): %s", message);
	SDL_Quit();
	throw std::logic_error(message);
}

// XXX: need to have a callback while initializing SDL, before any of the
//      engine audio stuff has been initialized, so this will fill the buffer
//      with zeros until another callback is set up
static void callbackStub(void *userdata, uint8_t *stream, int len) {
	context *ctx = reinterpret_cast<context*>(userdata);

	if (ctx && ctx->audioCallback) {
		ctx->audioCallback(ctx->callbackData, stream, len);

	} else {
		memset(stream, 0, len);
	}
}

context::context(const char *progname, const renderSettings& settings) {
	SDL_Log("got to context::context()");
	int flags
		= SDL_INIT_VIDEO
		| SDL_INIT_AUDIO
		| SDL_INIT_JOYSTICK
		| SDL_INIT_GAMECONTROLLER;

	if (SDL_Init(flags) < 0) {
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
		                         "Could not initialize",
		                         "Could not initialize SDL!", NULL);
		SDL_Die(SDL_GetError());
	}

	/*
	if (TTF_Init() < 0) {
		//SDL_Die("Couldn't initialize sdl2_ttf");
	}
	*/

#if GLSL_VERSION == 100
	// opengl es 2.0
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);

#elif GLSL_VERSION == 300
	// opengl es 3.0
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);

#elif GLSL_VERSION == 330
	// opengl core 3.3
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

#else
	// opengl core 4.3
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
#endif

	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	// stencil buffer for nanovg
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

	window = SDL_CreateWindow(progname, SDL_WINDOWPOS_CENTERED, 
	                         SDL_WINDOWPOS_CENTERED,
	                          32, 32,
	                          SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);

	if (!window) {
		SDL_Die("Couldn't create a window");
	}


	glcontext = SDL_GL_CreateContext(window);
	SDL_Log("have window + gl context");

	// apply settings after creating a GL context
	applySettings(settings);

	// TODO: would make more sense to have extension initialization stuff
	//       in initializeOpengl()
#if defined(__EMSCRIPTEN__)
	auto webgl = emscripten_webgl_get_current_context();

	bool half = emscripten_webgl_enable_extension(webgl, "EXT_color_buffer_half_float");
	bool full = emscripten_webgl_enable_extension(webgl, "EXT_color_buffer_float");
	bool nonsense = emscripten_webgl_enable_extension(webgl, "GL_OES_nonsense");

	SDL_Log("Have context: %d", webgl);
	SDL_Log("Extensions enabled: half float: %d, float: %d, nonsense: %d",
	        half, full, nonsense);
#endif

#if GLSL_VERSION >= 330
	// only use glew for core profiles
	if ((glew_status = glewInit()) != GLEW_OK) {
		SDL_Die("glewInit()");
		// TODO: check for vertex array extensions on gles2, error out
		//       if not supported
	}

	SDL_Log("initialized glew");
#endif

	SDL_AudioSpec want;
	SDL_memset(&want, 0, sizeof(want));
	want.freq = 44100;
	want.format = AUDIO_S16;
	want.channels = 2;
	want.samples = 2048;
	want.callback = callbackStub;
	want.userdata = this;

	audioOut = SDL_OpenAudioDevice(NULL, 0, &want, &audioHave, 0);

	if (audioOut == 0) {
		SDL_Die(SDL_GetError());

	} else {
		SDL_PauseAudioDevice(audioOut, 0);
	}

	SDL_Log("Finished initializing SDL");
}

context::~context() {
	SDL_Log("SDL done, cleaning up...");
	SDL_GL_DeleteContext(glcontext);
	SDL_DestroyWindow(window);
	SDL_CloseAudioDevice(audioOut);
	SDL_Quit();
}

void context::applySettings(const renderSettings& settings) {
	SDL_GL_SetSwapInterval(settings.vsync);

	if (settings.windowResX == 0 || settings.windowResY == 0) {
		//SDL_SetWindowSize(window, 1920, 1080);

		//SDL_SetWindowFullscreen(window, settings.fullscreen? SDL_WINDOW_FULLSCREEN : 0);
		SDL_SetWindowFullscreen(window, settings.fullscreen? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);

		if (settings.fullscreen) {
			// find best display mode in full screen mode
			// TODO: figure out how to find what desktop the window is on
			//       or let the user select the desktop
			int count = SDL_GetNumDisplayModes(0);
			if (count < 0) {
				SDL_Log("SDL_GetNumDisplayModes() failed...");
				return;
			}

			SDL_DisplayMode mode;
			SDL_DisplayMode best;
			uint32_t bestbpp = 0;

			memset(&best, 0, sizeof(best));

			for (int i = 0; i < count; i++) {
				if (SDL_GetDisplayMode(0, i, &mode) != 0) {
					SDL_Log("SDL_GetDisplayMode() failed...");
					return;
				}

				uint32_t f = mode.format;

				SDL_Log("Mode %d\tbpp: %d\t%s\t%dx%d",
						i, SDL_BITSPERPIXEL(f), SDL_GetPixelFormatName(f),
						mode.w, mode.h);

				if (mode.w > best.w && SDL_BITSPERPIXEL(f) >= bestbpp) {
					best = mode;
					bestbpp = SDL_BITSPERPIXEL(f);
				}
			}

			SDL_SetWindowDisplayMode(window, &best);

		} else {
			SDL_DisplayMode mode;
			if (SDL_GetDesktopDisplayMode(0, &mode) != 0) {
				SDL_SetWindowSize(window, mode.w, mode.h);

			} else {
				SDL_Log("SDL_GetDesktopDisplayMode() failed, defaulting to 1080p");
				SDL_SetWindowSize(window, 1920, 1080);
			}
		}

	} else {
		SDL_SetWindowSize(window, settings.windowResX, settings.windowResY);
	}

	SDL_Log("SDL: finished applying settings");
	/*
	SDL_ShowCursor(SDL_DISABLE);
	*/
}

void context::setAudioCallback(void *data, SDL_AudioCallback callback) {
	// Hmm, is locking needed here?
	callbackData = data;
	audioCallback = callback;
}

// namespace grendx
}
