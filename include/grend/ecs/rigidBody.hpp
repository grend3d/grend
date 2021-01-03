#pragma once

#include <grend/physics.hpp>
#include <grend/ecs/ecs.hpp>

namespace grendx::ecs {

class rigidBody : public component {
	public:
		rigidBody(entityManager *manager, entity *ent, float _mass)
			: component(manager, ent)
		{
			manager->registerComponent(ent, "rigidBody", this);
			mass = _mass;
		}

		virtual const char* typeString(void) const { return "rigidBody"; };
		virtual ~rigidBody() {
			std::cerr << "got to ~rigidBody()" << std::endl;
			//phys = nullptr;
			phys.reset();
		};

		void registerCollisionQueue(std::shared_ptr<std::vector<collision>> queue) {
			if (phys) {
				phys->collisionQueue = queue;
			}
			// TODO: should show warning if there's no physics object
		}

		physicsObject::ptr phys = nullptr;
		float mass;
};

class rigidBodySphere : public rigidBody {
	public:
		rigidBodySphere(entityManager *manager,
		                entity *ent,
		                glm::vec3 _position,
		                float _mass,
		                float _radius)
			: rigidBody(manager, ent, _mass),
			  position(_position),
			  mass(_mass),
			  radius(_radius)
		{
			manager->registerComponent(ent, "rigidBodySphere", this);
			//setRadius(radius);
			phys = manager->engine->phys->addSphere(ent, position, mass, radius);
		}

		virtual const char* typeString(void) const { return "rigidBodySphere"; };
		virtual ~rigidBodySphere() {
			std::cerr << "got to ~rigidBodySphere()" << std::endl;
		};

		virtual void setRadius(float r) {
			// XXX: if the object has moved since it was added, this will reset
			//      to position to where it was spawned...
			// TODO: see if bullet can resize the body, or grab the position
			//       from the old object if there was one
			//phys = manager->engine->phys->addSphere(ent, position, mass, r);
		}

		glm::vec3 position;
		float mass;
		float radius;
};

class rigidBodyBox : public rigidBody {
	public:
		rigidBodyBox(entityManager *manager,
		             entity *ent,
		             glm::vec3 position,
		             float mass,
		             AABBExtent& box)
			: rigidBody(manager, ent, mass)
		{
			manager->registerComponent(ent, "rigidBodyBox", this);
			phys = manager->engine->phys->addBox(ent, position, mass, box);
		}

		virtual const char* typeString(void) const { return "rigidBodyBox"; };
		virtual ~rigidBodyBox() {};
};

class syncRigidBody : public component {
	public:
		syncRigidBody(entityManager *manager, entity *ent)
			: component(manager, ent)
		{
			manager->registerComponent(ent, "syncRigidBody", this);
		}

		virtual const char* typeString(void) const { return "syncRigidBody"; };
		virtual ~syncRigidBody() {};
		virtual void sync(entityManager *manager, entity *ent) = 0;
};

class syncRigidBodyTransform : public syncRigidBody {
	public:
		syncRigidBodyTransform(entityManager *manager, entity *ent)
			: syncRigidBody(manager, ent)
		{
			manager->registerComponent(ent, "syncRigidBodyTransform", this);
		}

		virtual const char* typeString(void) const { return "syncRigidBodyTransform"; };
		virtual ~syncRigidBodyTransform() {};
		virtual void sync(entityManager *manager, entity *ent);
};

class syncRigidBodyPosition : public syncRigidBody {
	public:
		syncRigidBodyPosition(entityManager *manager, entity *ent)
			: syncRigidBody(manager, ent)
		{
			manager->registerComponent(ent, "syncRigidBodyPosition", this);
		}

		virtual const char* typeString(void) const { return "syncRigidBodyPosition"; };
		virtual ~syncRigidBodyPosition() {};
		virtual void sync(entityManager *manager, entity *ent);
};

class syncRigidBodyXZVelocity : public syncRigidBody {
	public:
		syncRigidBodyXZVelocity(entityManager *manager, entity *ent)
			: syncRigidBody(manager, ent)
		{
			manager->registerComponent(ent, "syncRigidBodyXZVelocity", this);
		}

		virtual const char* typeString(void) const { return "syncRigidBodyXZVelocity"; };
		virtual ~syncRigidBodyXZVelocity() {};
		virtual void sync(entityManager *manager, entity *ent);
};

class syncRigidBodySystem : public entitySystem {
	public:
		typedef std::shared_ptr<entitySystem> ptr;
		typedef std::weak_ptr<entitySystem>   weakptr;

		virtual void update(entityManager *manager, float delta);
};

// namespace grendx::ecs
};
