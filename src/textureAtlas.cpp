#include <grend/textureAtlas.hpp>
#include <grend/logger.hpp>

#include <stdio.h>
#include <iostream>

using namespace grendx;

atlas::atlas(size_t dimension, enum mode m) : tree(dimension) {
	framebuffer = genFramebuffer();
	framebuffer->bind();

	if (m == mode::Color) {
		color_tex = genTextureFormat(dimension, dimension, rgbaf_if_supported());
		framebuffer->attach(GL_COLOR_ATTACHMENT0, color_tex);
	}

	DO_ERROR_CHECK();

	depth_tex = genTextureFormat(dimension, dimension, depth_stencil_format());
	framebuffer->attach(GL_DEPTH_STENCIL_ATTACHMENT, depth_tex);
}

bool atlas::bind_atlas_fb(quadtree::node_id id) {
	auto info = tree.info(id);

	if (!info.valid) {
		LogFmt("quadtree node isn't valid????");
		return false;
	}

	/*
	fprintf(stderr, "### binding framebuffer for id %u, %ux%u + %u\n",
		id, info.x, info.y, info.size);
		*/

	//glman.bind_framebuffer(framebuffer);
	framebuffer->bind();
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
