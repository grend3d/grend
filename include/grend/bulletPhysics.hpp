#pragma once

#include <grend/glm-includes.hpp>
#include <grend/gameModel.hpp>
#include <grend/octree.hpp>
#include <grend/physics.hpp>
#include <grend/TRS.hpp>

#include <unordered_map>
#include <unistd.h>
#include <stdint.h>
#include <list>
#include <set>
#include <memory>

#include "btBulletDynamicsCommon.h"

namespace grendx {

class bulletObject : public physicsObject {
	friend class bulletPhysics;

	public:
		typedef std::shared_ptr<bulletObject> ptr;
		typedef std::weak_ptr<bulletObject>   weakptr;

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
		btRigidBody *body;
		btCollisionShape *shape;
		btDefaultMotionState *motionState;
		float mass;
};

class bulletPhysics : public physics {
	public:
		typedef std::shared_ptr<bulletPhysics> ptr;
		typedef std::weak_ptr<bulletPhysics>   weakptr;

		bulletPhysics();
		~bulletPhysics();

		// each return physics object ID
		// non-moveable geometry, collisions with octree
		virtual physicsObject::ptr
			addStaticModels(gameObject::ptr obj, const TRS& transform);

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
			addStaticMesh(gameObject::ptr obj,
			              const TRS& transform,
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

	private:
		btDefaultCollisionConfiguration *collisionConfig;
		btCollisionDispatcher *dispatcher;
		btBroadphaseInterface *pairCache;
		btSequentialImpulseConstraintSolver *solver;
		btDiscreteDynamicsWorld *world;

		std::set<physicsObject::ptr> objects;
};

// namespace grendx
}
