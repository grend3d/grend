#include <grend/sdlContext.hpp>
#include <stdexcept>
#include <iostream>

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

context::context(const char *progname) {
	SDL_Log("got to context::context()");
	int flags
		= SDL_INIT_VIDEO
		| SDL_INIT_AUDIO
		| SDL_INIT_JOYSTICK
		| SDL_INIT_GAMECONTROLLER;

	if (SDL_Init(flags) < 0) {
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
	                          SCREEN_SIZE_X, SCREEN_SIZE_Y,
	                          SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);


	/*
	SDL_ShowCursor(SDL_DISABLE);
	*/

	if (!window) {
		SDL_Die("Couldn't create a window");
	}

	glcontext = SDL_GL_CreateContext(window);
	SDL_Log("have window + gl context");

	SDL_GL_SetSwapInterval(1);

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
	SDL_GL_DeleteContext(glcontext);
	SDL_DestroyWindow(window);
	SDL_CloseAudioDevice(audioOut);
	SDL_Quit();
}

void context::setAudioCallback(void *data, SDL_AudioCallback callback) {
	// Hmm, is locking needed here?
	callbackData = data;
	audioCallback = callback;
}

// namespace grendx
}
