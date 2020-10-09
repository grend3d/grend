#pragma once

#include <grend/glm-includes.hpp>
#include <grend/gameModel.hpp>
#include <grend/octree.hpp>

#include <unordered_map>
#include <unistd.h>
#include <stdint.h>
#include <list>
#include <memory>

namespace grendx {

class physics {
	public: 
		typedef std::shared_ptr<physics> ptr;
		typedef std::weak_ptr<physics>   weakptr;

		struct collision {
			uint64_t a, b;
			glm::vec3 position;
			glm::vec3 normal;
			float depth;
		};

		uint64_t alloc_id(void) { return num_objects++; };
		// simple counter, never reuse IDs
		uint64_t num_objects = 0;

		// each return physics object ID
		// non-moveable geometry, collisions with octree
		virtual uint64_t add_static_models(gameObject::ptr obj,
		                                   glm::mat4 transform = glm::mat4(1)) = 0;

		// dynamic geometry, collisions with AABB tree
		virtual uint64_t add_sphere(gameObject::ptr obj, glm::vec3 pos,
		                            float mass, float r) = 0;
		virtual uint64_t add_box(gameObject::ptr obj, glm::vec3 pos,
		                        float mass, float length, float width,
		                        float height) = 0;

		// map of submesh name to physics object ID
		// TODO: multimap?
		virtual std::map<gameMesh::ptr, uint64_t>
			add_model_mesh_boxes(gameModel::ptr mod) = 0;
		virtual void remove(uint64_t id) = 0;
		virtual void clear(void) = 0;

		virtual void set_position(uint64_t id, glm::vec3 pos) = 0;
		virtual void set_velocity(uint64_t id, glm::vec3 vel) = 0;
		virtual glm::vec3 get_velocity(uint64_t id) = 0;
		virtual void set_acceleration(uint64_t id, glm::vec3 accel) = 0;
		virtual std::list<collision> find_collisions(float delta) = 0;
		virtual void step_simulation(float delta) = 0;
};

// namespace grendx
}
