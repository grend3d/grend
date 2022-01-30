#pragma once

#include <grend/physics.hpp>
#include <grend/ecs/ecs.hpp>

namespace grendx::ecs {

class transformUpdatable : public component {
	public:
		transformUpdatable(entityManager *manager, entity *ent)
			: component(manager, ent)
		{
			manager->registerComponent(ent, this);
		};

		transformUpdatable(entityManager *manager, entity *ent, nlohmann::json properties)
			: transformUpdatable(manager, ent) {};

		virtual ~transformUpdatable();

		virtual void setTransform(const TRS& transform) = 0;

		// serialization stuff
		constexpr static const char *serializedType = "transformUpdatable";
		virtual const char* typeString(void) const { return serializedType; };
};

class rigidBody : public transformUpdatable, public activatable {
	public:
		rigidBody(entityManager *manager, entity *ent, float _mass)
			: transformUpdatable(manager, ent)
		{
			manager->registerComponent(ent, this);
			manager->registerInterface<activatable>(ent, this);
			mass = _mass;
		}

		rigidBody(entityManager *manager,
		          entity *ent,
		          nlohmann::json properties);

		virtual ~rigidBody();

		virtual void setTransform(const TRS& transform) {
			if (phys) {
				phys->setTransform(transform);
			}
		}

		void registerCollisionQueue(std::shared_ptr<std::vector<collision>> queue) {
			if (phys) {
				phys->collisionQueue = queue;
			}
			// TODO: should show warning if there's no physics object
		}

		virtual void initBody(entityManager *manager, entity *ent) {
			// class needs to be instantiable for serialization,
			// so this can't be an abstract function, so note that
			// subclasses should implement this to generate a physics
			// object
		};

		// activation here means adding and deleting the physics object
		virtual void activate(entityManager *manager, entity *ent) {
			if (!phys) {
				initBody(manager, ent);
				// TODO: cached angular factor
				phys->setAngularFactor(cachedAngularFactor);
				phys->collisionQueue = cachedQueue;
			}
		}

		virtual void deactivate(entityManager *manager, entity *ent) {
			if (phys) {
				// TODO: cache angular factor
				position = phys->getTransform().position;
				cachedQueue = phys->collisionQueue;
				cachedAngularFactor = phys->getAngularFactor();
				phys->removeSelf();
				phys = nullptr;
			}
		}

		physicsObject::ptr phys = nullptr;
		float mass;
		float cachedAngularFactor = 1.f;
		glm::vec3 position;
		std::shared_ptr<std::vector<collision>> cachedQueue = nullptr;

		// serialization stuff
		constexpr static const char *serializedType = "rigidBody";
		virtual const char* typeString(void) const { return serializedType; };
};

class rigidBodySphere : public rigidBody {
	public:
		rigidBodySphere(entityManager *manager,
		                entity *ent,
		                glm::vec3 _position,
		                float _mass,
		                float _radius)
			: rigidBody(manager, ent, _mass)
		{
			//manager->registerComponent(ent, serializedType, this);
			manager->registerComponent(ent, this);

			position = _position;
			mass     = _mass;
			radius   = _radius;

			//setRadius(radius);
			//phys = manager->engine->phys->addSphere(ent, position, mass, radius);
			activate(manager, ent);
		}

		rigidBodySphere(entityManager *manager,
		                entity *ent,
		                nlohmann::json properties);

		virtual ~rigidBodySphere();

		virtual void initBody(entityManager *manager, entity *ent) {
			phys = manager->engine->phys->addSphere(ent, position, mass, radius);
		}

		virtual void setRadius(float r) {
			// XXX: if the object has moved since it was added, this will reset
			//      to position to where it was spawned...
			// TODO: see if bullet can resize the body, or grab the position
			//       from the old object if there was one
			//phys = manager->engine->phys->addSphere(ent, position, mass, r);
		}

		float radius;

		// serialization stuff
		constexpr static const char *serializedType = "rigidBodySphere";
		static const nlohmann::json defaultProperties(void) {
			auto ret = component::defaultProperties();

			ret.update({
				{"radius", 1.f},
			});

			return ret;
		};

		virtual const char* typeString(void) const { return serializedType; };
		virtual nlohmann::json serialize(entityManager *manager);
};

class rigidBodyBox : public rigidBody {
	public:
		rigidBodyBox(entityManager *manager,
		             entity *ent,
		             glm::vec3 _position,
		             float _mass,
		             AABBExtent& _extent)
			: rigidBody(manager, ent, _mass)
		{
			manager->registerComponent(ent, this);

			position = _position;
			mass     = _mass;
			extent   = _extent;

			activate(manager, ent);
		}

		rigidBodyBox(entityManager *manager,
		             entity *ent,
		             nlohmann::json properties);

		virtual ~rigidBodyBox();

		virtual void initBody(entityManager *manager, entity *ent) {
			phys = manager->engine->phys->addBox(ent, position, mass, extent);
		}

		AABBExtent extent;

		// serialization stuff
		constexpr static const char *serializedType = "rigidBodyBox";
		static const nlohmann::json defaultProperties(void) {
			auto ret = component::defaultProperties();

			ret.update({
				{"center", {0.f, 0.f, 0.f}},
				{"extent", {1.f, 1.f, 1.f}},
			});

			return ret;
		};

		virtual const char* typeString(void) const { return serializedType; };
		virtual nlohmann::json serialize(entityManager *manager);
};

// TODO: activatable
class rigidBodyCylinder : public rigidBody {
	public:
		rigidBodyCylinder(entityManager *manager,
		             entity *ent,
		             glm::vec3 _position,
		             float _mass,
		             AABBExtent& _extent)
			: rigidBody(manager, ent, mass)
		{
			manager->registerComponent(ent, this);

			position = _position;
			mass = _mass;
			extent = _extent;

			activate(manager, ent);
		}

		rigidBodyCylinder(entityManager *manager,
		             entity *ent,
		             nlohmann::json properties);

		virtual ~rigidBodyCylinder();

		virtual void initBody(entityManager *manager, entity *ent) {
			phys = manager->engine->phys->addCylinder(ent, position, mass, extent);
		}

		AABBExtent extent;

		// serialization stuff
		constexpr static const char *serializedType = "rigidBodyCylinder";
		static const nlohmann::json defaultProperties(void) {
			auto ret = component::defaultProperties();

			ret.update({
				{"center", {0.f, 0.f, 0.f}},
				{"extent", {1.f, 1.f, 1.f}},
			});

			return ret;
		};

		virtual const char* typeString(void) const { return serializedType; };
		virtual nlohmann::json serialize(entityManager *manager);
};

class rigidBodyCapsule : public rigidBody {
	public:
		rigidBodyCapsule(entityManager *manager,
		             entity *ent,
		             glm::vec3 _position,
		             float _mass,
		             float _radius,
		             float _height)
			: rigidBody(manager, ent, mass)
		{
			manager->registerComponent(ent, this);

			position = _position;
			mass     = _mass;
			radius   = _radius;
			height   = _height;

			activate(manager, ent);
		}

		rigidBodyCapsule(entityManager *manager,
		             entity *ent,
		             nlohmann::json properties);

		virtual ~rigidBodyCapsule();

		// for class activatable
		// activation here means adding and deleting the physics object
		virtual void initBody(entityManager *manager, entity *ent) {
			phys = manager->engine->phys->addCapsule(ent, position, mass, radius, height);
		}

		// position, mass in rigidBody
		float radius;
		float height;

		// serialization stuff
		constexpr static const char *serializedType = "rigidBodyCapsule";
		static const nlohmann::json defaultProperties(void) {
			auto ret = component::defaultProperties();

			ret.update({
				{"radius", 1.f},
				{"height", 2.f},
			});

			return ret;
		};

		virtual const char* typeString(void) const { return serializedType; };
		virtual nlohmann::json serialize(entityManager *manager);
};

class syncRigidBody : public component {
	public:
		constexpr static const char *serializedType = "syncRigidBody";

		syncRigidBody(entityManager *manager,
		              entity *ent,
		              nlohmann::json properties={})
			: component(manager, ent, properties)
		{
			manager->registerComponent(ent, this);
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
			manager->registerComponent(ent, this);
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
			manager->registerComponent(ent, this);
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
			manager->registerComponent(ent, this);
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

// TODO: Not sure if this is the best place for it, but this is the only thing
//       using transformUpdatable for now... Once I figure out what else would
//       need to go with it I'll move it into a seperate header
void updateEntityTransforms(entityManager *manager,
                            entity *ent,
                            const TRS& transform);

// namespace grendx::ecs
};
