#pragma once

#include <grend/sceneNode.hpp>
#include <grend/sceneModel.hpp>
#include <grend/glmIncludes.hpp>
#include <grend/TRS.hpp>
#include <grend/boundingBox.hpp>
#include <grend/IoC.hpp>

#include <unordered_map>
#include <unistd.h>
#include <stdint.h>
#include <list>
#include <memory>
#include <iostream>
#include <functional>

namespace grendx {

// forward declaration, sceneModel.hpp at end
class sceneModel;
class sceneMesh;

class physicsObject;

struct collision {
	std::shared_ptr<physicsObject> a, b;
	void *adata, *bdata;
	glm::vec3 position;
	glm::vec3 normal;
	float depth;
};

class physicsObject {
	public:
		typedef std::shared_ptr<physicsObject> ptr;
		typedef std::weak_ptr<physicsObject>   weakptr;

		virtual ~physicsObject();

		virtual void setTransform(const TRS& transform) = 0;
		virtual TRS  getTransform(void) = 0;

		virtual void setPosition(glm::vec3 pos) = 0;
		virtual void setVelocity(glm::vec3 vel) = 0;
		virtual void setAcceleration(glm::vec3 accel) = 0;
		virtual void setAngularFactor(float amount) = 0;
		virtual void setAngularFactor(const glm::vec3& amount) = 0;

		virtual glm::vec3 getPosition(void) {
			return getTransform().position;
		};

		virtual glm::vec3 getVelocity(void) = 0;
		virtual glm::vec3 getAcceleration(void) = 0;
		virtual glm::vec3 getAngularFactor(void) = 0;

		virtual void removeSelf(void) = 0;

		std::shared_ptr<std::vector<collision>> collisionQueue = nullptr;
};

class physics : public IoC::Service {
	public:
		typedef std::shared_ptr<physics> ptr;
		typedef std::weak_ptr<physics>   weakptr;

		virtual ~physics();

		virtual void drawDebug(glm::mat4 cam) {};
		virtual void setDebugMode(int mode) {};
		virtual size_t numObjects(void) = 0;

		/**
		 * Add non-movable collision geometry to the world.
		 *
		 * @param data User-defined pointer for storing arbitrary data attached to
		 *             this node. This can be a nullptr.
		 * @param obj  Root node to start adding from, all nodes are fully traversed
		 *             to find mesh objects.
		 * @param transform World-space transformation information.
		 * @param collector Collects shared pointers to resulting physics objects.
		 *                  these pointers should be stored for as long as you want
		 *                  the collision objects to persist.
		 * @param propFilter If not empty, this will only add mesh objects which
		 *                   have a property (in sceneNode::extraProperties) equal
		 *                   to the filter name.
		 */
		virtual void
		addStaticModels(void *data,
		                sceneNode::ptr obj,
		                const TRS& transform,
		                std::vector<physicsObject::ptr>& collector,
		                std::string propFilter = "") = 0;

		// add dynamic rigid bodies
		virtual physicsObject::ptr
		addSphere(void *data, glm::vec3 pos,
		          float mass, float r) = 0;

		virtual physicsObject::ptr
		addBox(void *data,
		       glm::vec3 position,
		       float mass,
		       AABBExtent& box) = 0;

		virtual physicsObject::ptr
		addCylinder(void *data,
		            glm::vec3 position,
		            float mass,
		            AABBExtent& box) = 0;

		virtual physicsObject::ptr
		addCapsule(void *data,
		           glm::vec3 position,
		           float mass,
		           float radius,
		           float height) = 0;

		virtual physicsObject::ptr
		addStaticMesh(void *data,
		              const TRS& transform,
		              sceneModel::ptr model,
		              sceneMesh::ptr  mesh) = 0;

		// map of submesh name to physics object ID
		// TODO: multimap?
		virtual std::map<sceneMesh::ptr, physicsObject::ptr>
		addModelMeshBoxes(sceneModel::ptr mod) = 0;

		virtual void remove(physicsObject::ptr obj) = 0;
		virtual void clear(void) = 0;

		virtual void filterCollisions(void) = 0;
		virtual void stepSimulation(float delta) = 0;
};

// namespace grendx
}

#include <grend/sceneModel.hpp>
