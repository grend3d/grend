#pragma once

#include <grend/glmIncludes.hpp>
#include <grend/gameModel.hpp>
#include <grend/octree.hpp>
#include <grend/physics.hpp>

#include <unordered_map>
#include <unistd.h>
#include <stdint.h>
#include <list>
#include <set>
#include <memory>

namespace grendx {

class impObject : public physicsObject {
	friend class impPhysics;

	public:
		typedef std::shared_ptr<impObject> ptr;
		typedef std::weak_ptr<impObject>   weakptr;

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

		enum type type;
		union { struct sphere usphere; struct box ubox; };

		virtual void setTransform(TRS& transform);
		virtual TRS  getTransform(void);

		virtual void setPosition(glm::vec3 pos);
		virtual void setVelocity(glm::vec3 vel);
		virtual void setAcceleration(glm::vec3 accel);
		virtual glm::vec3 getVelocity(void);
		virtual glm::vec3 getAcceleration(void);

	protected:
		gameObject::ptr obj;
		//std::string model_name;
		glm::vec3 position = {0, 0, 0};
		glm::vec3 velocity = {0, 0, 0};
		glm::vec3 acceleration = {0, 0, 0};
		glm::vec3 angularVelocity = {0, 0, 0};
		glm::quat rotation = {1, 0, 0, 0};
		float inverseMass = 0;
		float drag_s = 0.9;
		float gravity = -15.f;
};

class impPhysics : public physics {
	public:
		typedef std::shared_ptr<impPhysics> ptr;
		typedef std::weak_ptr<impPhysics>   weakptr;

		// each return physics object ID
		// non-moveable geometry, collisions with octree
		virtual physicsObject::ptr
			addStaticModels(gameObject::ptr obj,
		                    glm::mat4 transform = glm::mat4(1));

		// dynamic geometry, collisions with AABB tree
		virtual physicsObject::ptr
			addSphere(gameObject::ptr obj, glm::vec3 pos,
		              float mass, float r);

		virtual physicsObject::ptr
			addBox(gameObject::ptr obj,
			       glm::vec3 position,
			       float mass,
				   AABBExtent& box);

		virtual physicsObject::ptr
			addMesh(gameObject::ptr obj,
			        glm::vec3 position,
			        float mass,
			        gameModel::ptr model,
			        gameMesh::ptr mesh);

		// map of submesh name to physics object ID
		// TODO: multimap?
		virtual std::map<gameMesh::ptr, physicsObject::ptr>
			addModelMeshBoxes(gameModel::ptr mod);
		virtual void remove(physicsObject::ptr obj);
		virtual void clear(void);

		virtual size_t numObjects(void);
		virtual std::list<collision> findCollisions(float delta);
		virtual void stepSimulation(float delta);

		std::set<physicsObject::ptr> objects;
		octree static_geom;
};

// namespace grendx
}
