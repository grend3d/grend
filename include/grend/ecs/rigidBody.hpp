#pragma once

#include <grend/physics.hpp>
#include <grend/ecs/ecs.hpp>

namespace grendx::ecs {

class rigidBody : public component {
	public:
		constexpr static const char *serializedType = "rigidBody";

		rigidBody(entityManager *manager, entity *ent, float _mass)
			: component(manager, ent)
		{
			manager->registerComponent(ent, serializedType, this);
			mass = _mass;
		}

		rigidBody(entityManager *manager,
		          entity *ent,
		          nlohmann::json properties = {
		          	{"mass", 0.f},
		          }) 
			: component(manager, ent, properties)
		{
			// TODO
		}

		virtual ~rigidBody();
		virtual const char* typeString(void) const { return serializedType; };

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
		constexpr static const char *serializedType = "rigidBodySphere";

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
			manager->registerComponent(ent, serializedType, this);
			//setRadius(radius);
			phys = manager->engine->phys->addSphere(ent, position, mass, radius);
		}

		virtual ~rigidBodySphere();
		virtual const char* typeString(void) const { return serializedType; };

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
		constexpr static const char *serializedType = "rigidBodyBox";

		rigidBodyBox(entityManager *manager,
		             entity *ent,
		             glm::vec3 position,
		             float mass,
		             AABBExtent& box)
			: rigidBody(manager, ent, mass)
		{
			manager->registerComponent(ent, serializedType, this);
			phys = manager->engine->phys->addBox(ent, position, mass, box);
		}

		virtual ~rigidBodyBox();
		virtual const char* typeString(void) const { return serializedType; };
};

class syncRigidBody : public component {
	public:
		constexpr static const char *serializedType = "syncRigidBody";

		syncRigidBody(entityManager *manager,
		              entity *ent,
		              nlohmann::json properties={})
			: component(manager, ent, properties)
		{
			manager->registerComponent(ent, serializedType, this);
		}

		virtual ~syncRigidBody();
		virtual const char* typeString(void) const { return serializedType; };
		virtual void sync(entityManager *manager, entity *ent) = 0;
};

class syncRigidBodyTransform : public syncRigidBody {
	public:
		constexpr static const char *serializedType = "syncRigidBodyTransform";

		syncRigidBodyTransform(entityManager *manager,
		                       entity *ent,
		                       nlohmann::json properties={})
			: syncRigidBody(manager, ent, properties)
		{
			manager->registerComponent(ent, serializedType, this);
		}

		virtual ~syncRigidBodyTransform();
		virtual const char* typeString(void) const { return serializedType; };
		virtual void sync(entityManager *manager, entity *ent);
};

class syncRigidBodyPosition : public syncRigidBody {
	public:
		constexpr static const char *serializedType = "syncRigidBodyPosition";

		syncRigidBodyPosition(entityManager *manager,
		                      entity *ent,
		                      nlohmann::json properties={})
			: syncRigidBody(manager, ent, properties)
		{
			manager->registerComponent(ent, serializedType, this);
		}

		virtual ~syncRigidBodyPosition();
		virtual const char* typeString(void) const { return serializedType; };
		virtual void sync(entityManager *manager, entity *ent);
};

class syncRigidBodyXZVelocity : public syncRigidBody {
	public:
		constexpr static const char *serializedType = "syncRigidBodyXZVelocity";

		syncRigidBodyXZVelocity(entityManager *manager,
		                        entity *ent,
		                        nlohmann::json properties={})
			: syncRigidBody(manager, ent)
		{
			manager->registerComponent(ent, serializedType, this);
		}

		virtual ~syncRigidBodyXZVelocity();
		virtual const char* typeString(void) const { return serializedType; };
		virtual void sync(entityManager *manager, entity *ent);
};

class syncRigidBodySystem : public entitySystem {
	public:
		typedef std::shared_ptr<entitySystem> ptr;
		typedef std::weak_ptr<entitySystem>   weakptr;

		virtual ~syncRigidBodySystem();
		virtual void update(entityManager *manager, float delta);
};

// namespace grendx::ecs
};
