#pragma once
#include <grend/glm-includes.hpp>
#include <set>

#include <stdint.h>

namespace grendx {

class octree {
	public:
		class node;

		octree(double _leaf_size=0.05 /* meters */) {
			leaf_size = _leaf_size;
		};
		~octree() {};
		void grow(double size);
		void add_tri(const glm::vec3 tri[3]);
		bool collides(glm::vec3 position);
		void set_leaf(glm::vec3 location);

		node *root = nullptr;
		unsigned levels = 0;
		double leaf_size;
};

class octree::node {
	public:
		node *subnodes[2][2][2] = {0}; /* [x][y][z] */
		unsigned level;
};

// namespace grendx
}
