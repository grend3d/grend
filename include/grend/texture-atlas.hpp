#pragma once

#include <grend/quadtree.hpp>
#include <grend/glm-includes.hpp>
#include <grend/opengl-includes.hpp>
#include <grend/gl_manager.hpp>

namespace grendx {

class atlas {
	public:
		enum mode {
			Color,
			Depth,
		};

		atlas(gl_manager& man, size_t dimension, enum mode m = mode::Color);
		bool bind_atlas_fb(quadtree::node_id id);
		glm::mat3 tex_matrix(quadtree::node_id id);
		glm::vec3 tex_vector(quadtree::node_id id);

		quadtree tree;
		gl_manager::rhandle color_tex;
		gl_manager::rhandle depth_tex;

		// framebuffer with texture as a backing
		gl_manager::rhandle framebuffer;
		gl_manager& glman;
};

// namespace grendx
}
