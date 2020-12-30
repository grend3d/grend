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

		void registerCollisionQueue(std::shared_ptr<std::vector<collision>> queue) {
			if (phys) {
				phys->collisionQueue = queue;
			}
			// TODO: should show warning if there's no physics object
		}

		void syncPhysics(entity *ent) {
			if (ent && ent->node && phys) {
				ent->node->transform = phys->getTransform();
			}
		}

		physicsObject::ptr phys = nullptr;
		float mass;
};

class rigidBodySphere : public rigidBody {
	public:
		rigidBodySphere(entityManager *manager,
		                entity *ent,
		                glm::vec3 position,
		                float mass,
		                float radius)
			: rigidBody(manager, ent, mass)
		{
			manager->registerComponent(ent, "rigidBodySphere", this);
			phys = manager->engine->phys->addSphere(ent, position, mass, radius);
		}
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
};

class syncRigidBody : public component {
	public:
		syncRigidBody(entityManager *manager, entity *ent)
			: component(manager, ent)
		{
			manager->registerComponent(ent, "syncRigidBody", this);
		}

		virtual void sync(entityManager *manager, entity *ent) = 0;
};

class syncRigidBodyTransform : public syncRigidBody {
	public:
		syncRigidBodyTransform(entityManager *manager, entity *ent)
			: syncRigidBody(manager, ent)
		{
			manager->registerComponent(ent, "syncRigidBodyTransform", this);
		}

		virtual void sync(entityManager *manager, entity *ent);
};

class syncRigidBodyPosition : public syncRigidBody {
	public:
		syncRigidBodyPosition(entityManager *manager, entity *ent)
			: syncRigidBody(manager, ent)
		{
			manager->registerComponent(ent, "syncRigidBodyPosition", this);
		}

		virtual void sync(entityManager *manager, entity *ent);
};

class syncRigidBodyXZVelocity : public syncRigidBody {
	public:
		syncRigidBodyXZVelocity(entityManager *manager, entity *ent)
			: syncRigidBody(manager, ent)
		{
			manager->registerComponent(ent, "syncRigidBodyXZVelocity", this);
		}

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
