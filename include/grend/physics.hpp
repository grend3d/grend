#pragma once

#include <grend/gameObject.hpp>
#include <grend/glmIncludes.hpp>
#include <grend/TRS.hpp>
#include <grend/boundingBox.hpp>

#include <unordered_map>
#include <unistd.h>
#include <stdint.h>
#include <list>
#include <memory>
#include <iostream>
#include <functional>

namespace grendx {

// forward declaration, gameModel.hpp at end
class gameModel;
class gameMesh;

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
		virtual glm::vec3 getVelocity(void) = 0;
		virtual glm::vec3 getAcceleration(void) = 0;
		virtual void removeSelf(void) = 0;

		std::shared_ptr<std::vector<collision>> collisionQueue = nullptr;
};

class physics {
	public: 
		typedef std::shared_ptr<physics> ptr;
		typedef std::weak_ptr<physics>   weakptr;

		virtual ~physics();

		virtual void drawDebug(glm::mat4 cam) {};
		virtual void setDebugMode(int mode) {};
		virtual size_t numObjects(void) = 0;

		// add non-moveable geometry
		virtual void
		addStaticModels(void *data,
		                gameObject::ptr obj,
		                const TRS& transform,
		                std::vector<physicsObject::ptr>& collector) = 0;

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
		              std::shared_ptr<gameModel> model,
		              std::shared_ptr<gameMesh>  mesh) = 0;

		// map of submesh name to physics object ID
		// TODO: multimap?
		virtual std::map<std::shared_ptr<gameMesh>, physicsObject::ptr>
		addModelMeshBoxes(std::shared_ptr<gameModel> mod) = 0;

		virtual void remove(physicsObject::ptr obj) = 0;
		virtual void clear(void) = 0;

		virtual void filterCollisions(void) = 0;
		virtual void stepSimulation(float delta) = 0;
};

// namespace grendx
}

#include <grend/gameModel.hpp>
