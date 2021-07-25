#pragma once

#include <grend/glmIncludes.hpp>
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
#include "LinearMath/btIDebugDraw.h"
#include <mutex>
#include <condition_variable>

namespace grendx {

class bulletPhysics;
class bulletObject : public physicsObject {
	friend class bulletPhysics;

	public:
		typedef std::shared_ptr<bulletObject> ptr;
		typedef std::weak_ptr<bulletObject>   weakptr;

		virtual ~bulletObject();

		virtual void setTransform(const TRS& transform);
		virtual TRS  getTransform(void);

		virtual void setPosition(glm::vec3 pos);
		virtual void setVelocity(glm::vec3 vel);
		virtual void setAcceleration(glm::vec3 accel);
		virtual void setAngularFactor(float amount);
		virtual glm::vec3 getVelocity(void);
		virtual glm::vec3 getAcceleration(void);
		virtual void removeSelf(void);

	protected:
		gameObject::weakptr obj;
		bulletPhysics *runtime = nullptr;

		btRigidBody *body;
		btCollisionShape *shape;
		btDefaultMotionState *motionState;
		float mass;
		void *data;
};

#include <grend/glManager.hpp>
class bulletDebugDrawer : public btIDebugDraw {
	public:
		int currentMode = DebugDrawModes::DBG_NoDebug;
		// array of start, end, and color, each 3 floats
		struct vertex {
			glm::vec3 position;
			glm::vec3 color;
		};

		std::vector<vertex> lines;
		Buffer::ptr databuf;
		Vao::ptr vao;
		Program::ptr shader;

		bulletDebugDrawer();
		virtual ~bulletDebugDrawer();

		virtual void drawLine(const btVector3& from,
		                      const btVector3& to,
		                      const btVector3& color);

		virtual void drawContactPoint(const btVector3& onB,
		                              const btVector3& normalOnB,
		                              btScalar distance,
		                              int lifeTime,
		                              const btVector3& color) {};
		virtual void reportErrorWarning(const char *warning) {};
		virtual void draw3dText(const btVector3& location, const char *str) {};

		virtual void setDebugMode(int debugMode) { currentMode = debugMode; };
		virtual int getDebugMode() const { return currentMode; };

		virtual void clearLines() { lines.clear(); };
		// XXX: flushLines will need a view/world transform, nop overload
		virtual void flushLines() { /* lines.clear(); */ };
		virtual void flushLines(glm::mat4 cam);
};

class bulletPhysics : public physics {
	public:
		typedef std::shared_ptr<bulletPhysics> ptr;
		typedef std::weak_ptr<bulletPhysics>   weakptr;

		bulletPhysics();
		virtual ~bulletPhysics();
		virtual void drawDebug(glm::mat4 cam);
		virtual void setDebugMode(int mode);

		// each return physics object ID
		// non-moveable geometry, collisions with octree
		virtual void
		addStaticModels(void *data,
				gameObject::ptr obj,
				const TRS& transform,
				std::vector<physicsObject::ptr>& collector,
				std::string propFilter = "");

		// dynamic geometry, collisions with AABB tree
		virtual physicsObject::ptr
		addSphere(void *data, glm::vec3 pos,
		          float mass, float r);
		virtual physicsObject::ptr
		addBox(void *data,
		       glm::vec3 position,
		       float mass,
			   AABBExtent& box);

		virtual physicsObject::ptr
		addCylinder(void *data,
		            glm::vec3 position,
		            float mass,
		            AABBExtent& box);

		virtual physicsObject::ptr
		addCapsule(void *data,
		           glm::vec3 position,
		           float mass,
		           float radius,
		           float height);

		virtual physicsObject::ptr
		addStaticMesh(void *data,
		              const TRS& transform,
		              gameModel::ptr model,
		              gameMesh::ptr mesh);

		// map of submesh name to physics object ID
		// TODO: multimap?
		virtual std::map<gameMesh::ptr, physicsObject::ptr>
		addModelMeshBoxes(gameModel::ptr mod);

		virtual void remove(physicsObject::ptr obj);
		virtual void remove(bulletObject *ptr);
		virtual void clear(void);

		virtual size_t numObjects(void);
		virtual void filterCollisions(void);
		virtual void stepSimulation(float delta);

	private:
		btDefaultCollisionConfiguration *collisionConfig;
		btCollisionDispatcher *dispatcher;
		btBroadphaseInterface *pairCache;
		btSequentialImpulseConstraintSolver *solver;
		btDiscreteDynamicsWorld *world;

		std::map<bulletObject *, physicsObject::weakptr> objects;
		std::mutex bulletMutex;

		bulletDebugDrawer debugDrawer;
};

// namespace grendx
}
