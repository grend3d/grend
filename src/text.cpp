#include <grend/text.hpp>
#include <SDL_ttf.h>
#include <iostream>

using namespace grendx;

text_renderer::text_renderer(renderer *eng,
                             const char *font, int size)
	: rend(eng)
{
	// XXX: TODO asdf remove
	return;
	assert(rend != nullptr);

	if ((ttf = TTF_OpenFont(font, size)) == nullptr) {
		throw std::logic_error(std::string(__func__) + ": " + TTF_GetError());
	}

	text_vbo = rend->glman.gen_vbo();
	text_texture = rend->glman.gen_texture();

	gl_manager::rhandle orig_vao = rend->glman.current_vao;
	text_vao = rend->glman.bind_vao(rend->glman.gen_vao());

	rend->glman.bind_vbo(text_vbo, GL_ARRAY_BUFFER);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5*sizeof(float), 0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5*sizeof(float),
	                      (void*)(3 * sizeof(float)));

	gl_manager::rhandle vertex_shader = rend->glman.load_shader("shaders/out/UI.vert", GL_VERTEX_SHADER);
	gl_manager::rhandle fragment_shader = rend->glman.load_shader("shaders/out/UI.frag", GL_FRAGMENT_SHADER);
	text_shader = rend->glman.gen_program();

	glAttachShader(text_shader.first, vertex_shader.first);
	glAttachShader(text_shader.first, fragment_shader.first);
	glBindAttribLocation(text_shader.first, 0, "v_position");
	glBindAttribLocation(text_shader.first, 1, "v_texcoord");
	DO_ERROR_CHECK();

	//glBindAttribLocation(post_shader.first, glman.screenquad_vbo.first, "screenquad");
	DO_ERROR_CHECK();

	int linked;
	glLinkProgram(text_shader.first);
	glGetProgramiv(text_shader.first, GL_LINK_STATUS, &linked);

	if (!linked) {
		SDL_Die("couldn't link shaders");
	}

	rend->glman.bind_vao(orig_vao);
	DO_ERROR_CHECK();
}

text_renderer::~text_renderer() {
	// TODO: free vbo
}

void text_renderer::render(glm::vec3 pos,
                           std::string str,
                           SDL_Color color)
{
	rend->glman.bind_vao(text_vao);
	rend->set_shader(text_shader);

	// TODO: make this a parameter
	//SDL_Surface *surf = TTF_RenderUTF8_Solid(ttf, str.c_str(), {255, 255, 0, 255});
	//SDL_Surface *surf = TTF_RenderUTF8_Shaded(ttf, str.c_str(), {255, 255, 0}, {0, 0, 0});
	SDL_Surface *surf = TTF_RenderUTF8_Blended(ttf, str.c_str(), {255, 255, 0, 255});

	if (surf == nullptr) {
		// TODO: signal error somehow
		std::cerr << __func__ << "couldn't render text" << std::endl;
		return;
	}

	SDL_Surface *rgba_surf =
		SDL_ConvertSurfaceFormat(surf, SDL_PIXELFORMAT_RGBA32, 0);
	SDL_LockSurface(rgba_surf);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, text_texture.first);
	DO_ERROR_CHECK();

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, rgba_surf->w, rgba_surf->h, 0,
	             surface_gl_format(rgba_surf), GL_UNSIGNED_BYTE, rgba_surf->pixels);
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

	/*
	fprintf(stderr, " > loaded text: w = %u, h = %u, pitch = %u, bytesperpixel: %u\n",
	        rgba_surf->w, rgba_surf->h, rgba_surf->pitch, rgba_surf->format->BytesPerPixel);
			*/

	glUniform1i(glGetUniformLocation(rend->shader.first, "UItex"), 0);

	rend->glman.buffer_vbo(text_vbo, GL_ARRAY_BUFFER, quad, GL_DYNAMIC_DRAW);
	glDrawArrays(GL_TRIANGLES, 0, 6);

	SDL_FreeSurface(rgba_surf);
	SDL_FreeSurface(surf);
}
