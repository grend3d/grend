#include <grend/text.hpp>
//#include <SDL_ttf.h>
#include <iostream>

using namespace grendx;

text_renderer::text_renderer(renderContext::ptr eng,
                             const char *font, int size)
	: rend(eng)
{
	// XXX: TODO asdf remove
	return;
	/*
	assert(rend != nullptr);

	if ((ttf = TTF_OpenFont(font, size)) == nullptr) {
		throw std::logic_error(std::string(__func__) + ": " + TTF_GetError());
	}

	text_vbo = genBuffer(GL_ARRAY_BUFFER);
	text_texture = genTexture();

	Vao::ptr orig_vao = getCurrentVao();
	text_vao = bindVao(genVao());

	text_vbo->bind();
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5*sizeof(float), 0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5*sizeof(float),
	                      (void*)(3 * sizeof(float)));

	shaderOptions nullopts;
	text_shader = loadProgram(
		"shaders/out/UI.vert",
		"shaders/out/UI.frag",
		nullopts
	);

	text_shader->attribute("v_position", 0);
	text_shader->attribute("v_texcoord", 1);
	text_shader->link();

	bindVao(orig_vao);
	DO_ERROR_CHECK();
	*/
}

text_renderer::~text_renderer() {
	// TODO: free vbo
}

void text_renderer::render(glm::vec3 pos,
                           std::string str,
                           SDL_Color color)
{
	/*
	bindVao(text_vao);
	text_shader->bind();
	//rend->set_shader(text_shader);

	// TODO: make this a parameter
	//SDL_Surface *surf = TTF_RenderUTF8_Solid(ttf, str.c_str(), {255, 255, 0, 255});
	//SDL_Surface *surf = TTF_RenderUTF8_Shaded(ttf, str.c_str(), {255, 255, 0}, {0, 0, 0});
	SDL_Surface *surf = TTF_RenderUTF8_Blended(ttf, str.c_str(), {255, 255, 0, 255});

	if (surf == nullptr) {
		// TODO: signal error somehow
		return;
	}

	SDL_Surface *rgba_surf =
		SDL_ConvertSurfaceFormat(surf, SDL_PIXELFORMAT_RGBA32, 0);
	SDL_LockSurface(rgba_surf);

	glActiveTexture(GL_TEXTURE0);
	text_texture->bind();
	DO_ERROR_CHECK();

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, rgba_surf->w, rgba_surf->h, 0,
	             surfaceGlFormat(rgba_surf), GL_UNSIGNED_BYTE, rgba_surf->pixels);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	DO_ERROR_CHECK();

	GLfloat width = (1.f*rgba_surf->w) / SCREEN_SIZE_X;
	GLfloat height = (1.f*rgba_surf->h) / SCREEN_SIZE_Y;

	std::vector<GLfloat> quad = {
		 pos.x,         pos.y,          0, 0, 1,
		 pos.x + width, pos.y,          0, 1, 1,
		 pos.x + width, pos.y + height, 0, 1, 0,

		 pos.x + width, pos.y + height, 0, 1, 0,
		 pos.x,         pos.y + height, 0, 0, 0,
		 pos.x,         pos.y,          0, 0, 1,
	};
	*/

	/*
	fprintf(stderr, " > loaded text: w = %u, h = %u, pitch = %u, bytesperpixel: %u\n",
	        rgba_surf->w, rgba_surf->h, rgba_surf->pitch, rgba_surf->format->BytesPerPixel);
			*/

	/*
	text_shader->set("UItex", 0);
	text_vbo->buffer(quad);
	glDrawArrays(GL_TRIANGLES, 0, 6);

	SDL_FreeSurface(rgba_surf);
	SDL_FreeSurface(surf);
	*/
}
