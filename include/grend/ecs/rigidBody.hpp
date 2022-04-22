#pragma once

#include <grend/physics.hpp>
#include <grend/ecs/ecs.hpp>

namespace grendx::ecs {

// interface
class transformUpdatable {
	public:
		virtual ~transformUpdatable();
		virtual void setTransform(const TRS& transform) = 0;
};

class rigidBody
	: public component,
      public transformUpdatable,
      public activatable
{
	public:
		rigidBody(regArgs t, float mass);
		rigidBody(regArgs t);

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

			cachedQueue = queue;
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
		float mass = 0.f;
		float cachedAngularFactor = 1.f;
		glm::vec3 position;
		std::shared_ptr<std::vector<collision>> cachedQueue = nullptr;

		// serialization stuff
		virtual const char* typeString(void) const { return getTypeName(*this); };

		static nlohmann::json serializer(component *comp) {
			rigidBody *body = static_cast<rigidBody*>(comp);

			auto& p = body->position;
			return {
				{"mass",     body->mass},
				{"position", {p.x, p.y, p.z}},
			};
		}

		static void deserializer(component *comp, nlohmann::json& j) {
			rigidBody *body = static_cast<rigidBody*>(comp);

			std::array<float, 3> p = j.contains("position")
				? j["position"].get<std::array<float, 3>>()
				: std::array<float, 3> {0, 0, 0};

			body->mass = j.contains("mass")? j["mass"].get<float>() : 0.f;
			body->position = {p[0], p[1], p[2]};
		}
};

class rigidBodySphere : public rigidBody {
	public:
		rigidBodySphere(regArgs t,
		                const glm::vec3& _position,
		                float _mass,
		                float _radius)
			: rigidBody(doRegister(this, t), _mass)
		{
			//manager->registerComponent(ent, serializedType, this);
			//manager->registerComponent(ent, this);

			position = _position;
			mass     = _mass;
			radius   = _radius;

			//setRadius(radius);
			//phys = manager->engine->phys->addSphere(ent, position, mass, radius);
			activate(t.manager, t.ent);
		}

		rigidBodySphere(regArgs t);

		virtual ~rigidBodySphere();
		virtual const char* typeString(void) const { return getTypeName(*this); };

		virtual void initBody(entityManager *manager, entity *ent) {
			phys = manager->engine->phys->addSphere(ent, position, mass, radius);
		}

		virtual void setRadius(float r) {
			// XXX: if the object has moved since it was added, this will reset
			//      to position to where it was spawned...
			// TODO: see if bullet can resize the body, or grab the position
			//       from the old object if there was one
			// NOTE: this is easy to do now that activate/deactivate are present,
			//       just deactivate, set radius, reactivate, but see if
			//       bullet can just resize, that seems better
			//phys = manager->engine->phys->addSphere(ent, position, mass, r);
		}

		float radius = 1.f;
};

class rigidBodyBox : public rigidBody {
	public:
		rigidBodyBox(regArgs t,
		             const glm::vec3& _position,
		             float _mass,
		             const AABBExtent& _extent)
			: rigidBody(doRegister(this, t), _mass)
		{
			position = _position;
			mass     = _mass;
			extent   = _extent;

			activate(t.manager, t.ent);
		}

		rigidBodyBox(regArgs t);

		virtual ~rigidBodyBox();
		virtual const char* typeString(void) const { return getTypeName(*this); };

		virtual void initBody(entityManager *manager, entity *ent) {
			phys = manager->engine->phys->addBox(ent, position, mass, extent);
		}

		AABBExtent extent = {
			.center = {0, 0, 0},
			.extent = {1, 1, 1},
		};
};

// TODO: activatable
class rigidBodyCylinder : public rigidBody {
	public:
		rigidBodyCylinder(regArgs t,
		                  const glm::vec3& _position,
		                  float _mass,
		                  const AABBExtent& _extent)
			: rigidBody(doRegister(this, t), mass)
		{
			position = _position;
			mass = _mass;
			extent = _extent;

			activate(t.manager, t.ent);
		}

		rigidBodyCylinder(regArgs t);

		virtual ~rigidBodyCylinder();
		virtual const char* typeString(void) const { return getTypeName(*this); };

		virtual void initBody(entityManager *manager, entity *ent) {
			phys = manager->engine->phys->addCylinder(ent, position, mass, extent);
		}

		AABBExtent extent = {
			.center = {0, 0, 0},
			.extent = {1, 1, 1},
		};
};

class rigidBodyCapsule : public rigidBody {
	public:
		rigidBodyCapsule(regArgs t,
		                 const glm::vec3& _position,
		                 float _mass,
		                 float _radius,
		                 float _height)
			: rigidBody(doRegister(this, t), mass)
		{
			position = _position;
			mass     = _mass;
			radius   = _radius;
			height   = _height;

			activate(t.manager, t.ent);
		}

		rigidBodyCapsule(regArgs t);

		virtual ~rigidBodyCapsule();
		virtual const char* typeString(void) const { return getTypeName(*this); };

		// for class activatable
		// activation here means adding and deleting the physics object
		virtual void initBody(entityManager *manager, entity *ent) {
			phys = manager->engine->phys->addCapsule(ent, position, mass, radius, height);
		}

		// position, mass in rigidBody
		float radius = 1.f;
		float height = 1.f;
};

class syncRigidBody : public component {
	public:
		syncRigidBody(regArgs t)
			: component(doRegister(this, t)) {}

		virtual ~syncRigidBody();
		virtual const char* typeString(void) const { return getTypeName(*this); };

		virtual void sync(entityManager *manager, entity *ent) = 0;
};

class syncRigidBodyTransform : public syncRigidBody {
	public:
		syncRigidBodyTransform(regArgs t)
			: syncRigidBody(doRegister(this, t)) {}

		virtual ~syncRigidBodyTransform();
		virtual const char* typeString(void) const { return getTypeName(*this); };

		virtual void sync(entityManager *manager, entity *ent);
};

class syncRigidBodyPosition : public syncRigidBody {
	public:
		syncRigidBodyPosition(regArgs t)
			: syncRigidBody(doRegister(this, t)) {}

		virtual ~syncRigidBodyPosition();
		virtual const char* typeString(void) const { return getTypeName(*this); };

		virtual void sync(entityManager *manager, entity *ent);
};

class syncRigidBodyXZVelocity : public syncRigidBody {
	public:
		syncRigidBodyXZVelocity(regArgs t)
			: syncRigidBody(doRegister(this, t)) {}

		virtual ~syncRigidBodyXZVelocity();
		virtual const char* typeString(void) const { return getTypeName(*this); };

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
