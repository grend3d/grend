#pragma once

#include <grend/glm-includes.hpp>
#include <grend/gameModel.hpp>
#include <grend/octree.hpp>
#include <grend/physics.hpp>

#include <unordered_map>
#include <unistd.h>
#include <stdint.h>
#include <list>
#include <memory>

namespace grendx {

class imp_physics : public physics {
	public: 
		typedef std::shared_ptr<imp_physics> ptr;
		typedef std::weak_ptr<imp_physics>   weakptr;

		struct object {
			enum type {
				Static,
				Sphere,
				Box,
				Mesh,
			};

			struct sphere {
				float radius;
			};

			struct box {
				float length;
				float width;
				float height;
				// TODO: rotation, for boxes that closely fit the mesh
			};

			uint64_t id;

			enum type type;
			union { struct sphere usphere; struct box ubox; };

			gameObject::ptr obj;
			std::string model_name;
			glm::vec3 position = {0, 0, 0};
			glm::vec3 velocity = {0, 0, 0};
			glm::vec3 acceleration = {0, 0, 0};
			glm::vec3 angular_velocity = {0, 0, 0};
			glm::quat rotation = {1, 0, 0, 0};
			float inverse_mass = 0;
			float drag_s = 0.9;
			float gravity = -15.f;
		};

		// each return physics object ID
		// non-moveable geometry, collisions with octree
		virtual uint64_t add_static_models(gameObject::ptr obj,
		                                   glm::mat4 transform = glm::mat4(1));

		// dynamic geometry, collisions with AABB tree
		virtual uint64_t add_sphere(gameObject::ptr obj, glm::vec3 pos,
		                            float mass, float r);
		virtual uint64_t add_box(gameObject::ptr obj, glm::vec3 pos,
		                        float mass, float length, float width,
		                        float height);

		// map of submesh name to physics object ID
		// TODO: multimap?
		virtual std::map<gameMesh::ptr, uint64_t>
			add_model_mesh_boxes(gameModel::ptr mod);
		virtual void remove(uint64_t id);
		virtual void clear(void);

		virtual void set_acceleration(uint64_t id, glm::vec3 accel);
		virtual std::list<collision> find_collisions(float delta);
		virtual void step_simulation(float delta);

		std::unordered_map<uint64_t, struct object> objects;
		octree static_geom;
};

// namespace grendx
}
