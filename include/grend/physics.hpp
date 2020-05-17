#pragma once

#include <grend/glm-includes.hpp>
#include <grend/model.hpp>
#include <grend/octree.hpp>

#include <unordered_map>
#include <unistd.h>
#include <stdint.h>
#include <list>

namespace grendx {

class imp_physics {
	public: 
		class object {
			public:
				enum type {
					Static,
					Sphere,
					Box,
				};

				struct sphere {
					double radius;
				};

				struct box {
					double length;
					double width;
					double height;
					// TODO: rotation, for boxes that closely fit the mesh
				};

				uint64_t id;

				enum type type;
				union { struct sphere usphere; struct box ubox; };

				std::string model_name;
				glm::vec3 position = {0, 0, 0};
				glm::vec3 velocity = {0, 0, 0};
				glm::vec3 acceleration = {0, 0, 0};
				glm::vec3 angular_velocity = {0, 0, 0};
				glm::quat rotation = {0, 0, 0, 0};
				float inverse_mass = 0;
				float drag_s = 0.9;
				float gravity = -15.f;
		};

		struct collision {
			uint64_t a, b;
			glm::vec3 position;
			glm::vec3 normal;
			float depth;
		};

		// each return physics object ID
		// non-moveable geometry, collisions with octree
		uint64_t add_static_model(std::string modname, model& mod,
		                          glm::mat4 transform);

		// dynamic geometry, collisions with AABB tree
		uint64_t add_sphere(std::string modname, glm::vec3 pos, double r);
		uint64_t add_box(std::string modname, glm::vec3 pos,
		                 double length, double width, double height);
		// map of submesh name to physics object ID
		// TODO: multimap?
		std::map<std::string, uint64_t> add_model_mesh_boxes(model& mod);
		void remove(uint64_t id);

		void set_acceleration(uint64_t id, glm::vec3 accel);

		std::list<collision> find_collisions(float delta);
		void solve_contraints(float delta);
		uint64_t alloc_id(void) { return num_objects++; };

		std::unordered_map<uint64_t, struct object> objects;
		// simple counter, never reuse IDs
		uint64_t num_objects = 0;
		octree static_geom;

};

// namespace grendx
}
