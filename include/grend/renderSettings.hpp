#pragma once

namespace grendx {

// user-facing config, eg. texture, shadow quality, antialiasing, scaling...
struct renderSettings {
	// Internal renderer settings
	//
	// thought: atlas, subtexture sizes all need to be powers of two,
	//          would it make more sense to store only the exponent?
	bool shadowsEnabled = true;
	unsigned shadowSize = 256;
	unsigned shadowAtlasSize = 4096;

	// TODO: might be modal, for eg. screenspace shadows
	bool reflectionsEnabled = true;
	unsigned reflectionSize = 256;
	unsigned reflectionAtlasSize = 4096;

	bool lightProbesEnabled = true;
	unsigned lightProbeSize = 16;
	unsigned lightProbeAtlasSize = 1024;

	float scaleX = 1.0;
	float scaleY = 1.0;
	bool dynamicScaling = false;
	// TODO: redo the dynamic scaling stuff

	// Hmm... -1 for window resolution?
	unsigned targetResX = 1920; /* internal resolution is this * scale */
	unsigned targetResY = 1080;
	unsigned msaaLevel              = 4; /* 0 is off */
	unsigned anisotropicFilterLevel = 2; /* 0 is off */

	bool postprocessing = true;

	// SDL-side settings
	bool fullscreen = false;
	unsigned windowResX = 0; // 0 defaults to largest
	unsigned windowResY = 0; // 0 defaults to largest
	int vsync = 1;    // input to SDL_GL_SetSwapInterval()
	int display = -1; // any display TODO: mode listing
	// TODO: show/hide cursor setting here?

	// UI settings
	float UIScale = 1.0;
};

// namespace grendx
}
