#pragma once
#include <grend/glmIncludes.hpp>
#include <grend/sceneModel.hpp>
#include <utility>
#include <set>

#include <stdint.h>

namespace grendx {

class octree {
	public:
		// // (not yet) 'double' is depth of collision (0 for non-colliding),
		// 'bool' is whether there's a collision
		// 'vec3' is the normal for the colliding node
		typedef std::pair<float, glm::vec3> collision;
		class node;

		octree(double _leaf_size=0.1 /* meters */) {
			leaf_size = _leaf_size;
		};
		~octree() {};


		// TODO: clear()
		void grow(double size);
		void add_tri(const glm::vec3 tri[3], const glm::vec3 normals[3]);
		void add_model(sceneModel::ptr mod, glm::mat4 transform);
		void set_leaf(glm::vec3 location, glm::vec3 normal);
		node *get_leaf(glm::vec3 location);
		uint32_t count_nodes(void);

		collision collides(glm::vec3 begin, glm::vec3 end);
		collision collides_sphere(glm::vec3 position, float radius);

		node *root = nullptr;
		unsigned levels = 0;
		double leaf_size;
};

class octree::node {
	public:
		node() {
			for (unsigned i = 0; i < 8; i++) {
				subnodes[!!(i&1)][!!(i&2)][!!(i&4)] = nullptr;
			}
		}
		uint32_t count_nodes(void);
		node *subnodes[2][2][2]; /* [x][y][z] */
		unsigned level = 0;

		glm::vec3 normals = {0, 0, 0};
		size_t normal_samples = 0;
};

// namespace grendx
}
