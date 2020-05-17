#pragma once

#include <grend/gl_manager.hpp>
#include <grend/sdl-context.hpp>
#include <grend/engine.hpp>
#include <SDL_ttf.h>

namespace grendx {

class text_renderer {
	public:
		text_renderer(engine *eng,
		             const char *font="assets/fonts/droid-sans-mono-zeromod/DroidSansMonoSlashed.ttf",
		             int size=32);
		~text_renderer();

		void render(glm::vec3 pos, std::string str, SDL_Color color = {255, 255, 255, 255});

		TTF_Font *ttf = nullptr;
		engine *gengine;
		gl_manager::rhandle text_vbo;
		gl_manager::rhandle text_vao;
		gl_manager::rhandle text_texture;
		gl_manager::rhandle text_shader;

		// TODO: font texture render cache
		gl_manager::rhandle cache_texture;
};

// namespace grendx
}
