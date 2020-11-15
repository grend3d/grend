#pragma once

#include <grend/glmIncludes.hpp>
#include <grend/gameModel.hpp>
#include <grend/TRS.hpp>
#include <grend/octree.hpp>
#include <grend/boundingBox.hpp>

#include <unordered_map>
#include <unistd.h>
#include <stdint.h>
#include <list>
#include <memory>

namespace grendx {

class physicsObject {
	public:
		typedef std::shared_ptr<physicsObject> ptr;
		typedef std::weak_ptr<physicsObject>   weakptr;

		virtual void setTransform(TRS& transform) = 0;
		virtual TRS  getTransform(void) = 0;

		virtual void setPosition(glm::vec3 pos) = 0;
		virtual void setVelocity(glm::vec3 vel) = 0;
		virtual void setAcceleration(glm::vec3 accel) = 0;
		virtual glm::vec3 getVelocity(void) = 0;
		virtual glm::vec3 getAcceleration(void) = 0;
};

class physics {
	public: 
		typedef std::shared_ptr<physics> ptr;
		typedef std::weak_ptr<physics>   weakptr;

		struct collision {
			physicsObject::ptr a, b;
			glm::vec3 position;
			glm::vec3 normal;
			float depth;
		};

		virtual size_t numObjects(void) = 0;

		// add non-moveable geometry
		virtual physicsObject::ptr
		    addStaticModels(gameObject::ptr obj, const TRS& transform) = 0;

		// add dynamic rigid bodies
		virtual physicsObject::ptr
			addSphere(gameObject::ptr obj, glm::vec3 pos,
		              float mass, float r) = 0;
		virtual physicsObject::ptr
			addBox(gameObject::ptr obj,
			       glm::vec3 position,
			       float mass,
				   AABBExtent& box) = 0;
		virtual physicsObject::ptr
			addStaticMesh(gameObject::ptr obj,
			              const TRS& transform,
			              gameModel::ptr model,
			              gameMesh::ptr mesh) = 0;


		// map of submesh name to physics object ID
		// TODO: multimap?
		virtual std::map<gameMesh::ptr, physicsObject::ptr>
			addModelMeshBoxes(gameModel::ptr mod) = 0;
		virtual void remove(physicsObject::ptr obj) = 0;
		virtual void clear(void) = 0;

		virtual std::list<collision> findCollisions(float delta) = 0;
		virtual void stepSimulation(float delta) = 0;
};

// namespace grendx
}
