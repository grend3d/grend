#include <grend/texture-atlas.hpp>
#include <stdio.h>
#include <iostream>

using namespace grendx;

atlas::atlas(gl_manager& man, size_t dimension, enum mode m)
	: tree(dimension), glman(man)
{
	framebuffer = glman.gen_framebuffer();
	glman.bind_framebuffer(framebuffer);

	if (m == mode::Color) {
		color_tex = glman.fb_attach_texture(GL_COLOR_ATTACHMENT0,
				glman.gen_texture_color(dimension, dimension));
	}

	depth_tex = glman.fb_attach_texture(GL_DEPTH_STENCIL_ATTACHMENT, 
			glman.gen_texture_depth_stencil(dimension, dimension));
}

bool atlas::bind_atlas_fb(quadtree::node_id id) {
	auto info = tree.info(id);

	if (!info.valid) {
		std::cerr << "????" << std::endl;
		return false;
	}

	/*
	fprintf(stderr, "### binding framebuffer for id %u, %ux%u + %u\n",
		id, info.x, info.y, info.size);
		*/

	glman.bind_framebuffer(framebuffer);
	glViewport(info.x, info.y, info.size, info.size);
	glScissor(info.x, info.y, info.size, info.size);

	return true;
}

glm::mat3 atlas::tex_matrix(quadtree::node_id id) {
	quadinfo info = tree.info(id);

	if (!info.valid) {
		return glm::mat3(0);
	}

	float dim = info.dimension;
	float sz = info.size;
	float px = info.x;
	float py = info.y;

	glm::mat3 trans(
		0, 0, 0,
		0, 0, 0,
		px/dim, py/dim, 1.f
	);

	glm::mat3 scale(
		sz/dim, 0, 0,
		0, sz/dim, 0,
		0, 0, 1.0f
	);

	return trans * scale;
}

glm::vec3 atlas::tex_vector(quadtree::node_id id) {
	quadinfo info = tree.info(id);

	if (!info.valid) {
		return glm::vec3(0);
	}

	float dim = info.dimension;
	float sz = info.size;
	float px = info.x;
	float py = info.y;

	return {px/dim, py/dim, sz/dim};
}
