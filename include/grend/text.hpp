#pragma once

#include <grend/gl_manager.hpp>
#include <grend/sdl-context.hpp>
#include <grend/engine.hpp>
#include <SDL_ttf.h>

namespace grendx {

class text_renderer {
	public:
		text_renderer(renderer *arend,
		             const char *font="assets/fonts/droid-sans-mono-zeromod/DroidSansMonoSlashed.ttf",
		             int size=32);
		~text_renderer();

		void render(glm::vec3 pos, std::string str, SDL_Color color = {255, 255, 255, 255});

		TTF_Font *ttf = nullptr;
		renderer *rend;
		Vbo::ptr     text_vbo;
		Vao::ptr     text_vao;
		Texture::ptr text_texture;
		Program::ptr text_shader;

		// TODO: font texture render cache
		Texture::ptr cache_texture;
};

// namespace grendx
}
