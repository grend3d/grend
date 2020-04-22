#pragma once
#include <grend/glm-includes.hpp>
#include <grend/model.hpp>
#include <set>

#include <stdint.h>

namespace grendx {

class octree {
	public:
		class node;

		octree(double _leaf_size=0.02 /* meters */) {
			leaf_size = _leaf_size;
		};
		~octree() {};

		void grow(double size);
		void add_tri(const glm::vec3 tri[3]);
		void add_model(const model& mod, glm::mat4 transform);
		void set_leaf(glm::vec3 location);
		uint32_t count_nodes(void);

		bool collides(glm::vec3 position);
		bool collides_sphere(glm::vec3 position, float radius);

		node *root = nullptr;
		unsigned levels = 0;
		double leaf_size;
};

class octree::node {
	public:
		uint32_t count_nodes(void);
		node *subnodes[2][2][2] = {0}; /* [x][y][z] */
		unsigned level;
};

// namespace grendx
}
