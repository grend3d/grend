#pragma once

#include <grend/gameObject.hpp>
#include <grend/gameMain.hpp>
#include <grend/ecs/ecs.hpp>
#include <grend/ecs/rigidBody.hpp>
#include <grend/ecs/collision.hpp>
#include <grend/ecs/serializer.hpp>

namespace grendx::ecs {

class rigidBodySphereSerializer : public componentSerializer {
	public:
		constexpr static const char *serializedType = "rigidBodySphere";

		virtual nlohmann::json serialize(component *comp) {
			rigidBodySphere *body = dynamic_cast<rigidBodySphere*>(comp);

			if (body) {
				return {{"radius", body->radius}};

			} else {
				return {};
			}
		}

		virtual void deserialize(component *comp, nlohmann::json source) {
			rigidBodySphere *body = dynamic_cast<rigidBodySphere*>(comp);

			if (body && source.contains("radius")) {
				body->setRadius(source["radius"]);
			}
		}

		virtual component *allocate(entityManager *manager,
		                            entity *ent,
		                            nlohmann::json source)
		{
			// TODO: check
			auto radius = source["radius"];
			auto mass   = source["mass"];
			auto posvec = source["position"];

			// TODO: what if you don't want to place the rigid body at the
			//       entity's position?
			//       might be better to specify an offset relative to the entity
			return new rigidBodySphere(manager, ent, ent->node->transform.position,
			                           mass, radius);
		}
};

class syncRigidBodyTransformSerializer : public componentSerializer {
	public:
		constexpr static const char *serializedType = "syncRigidBodyTransform";

		virtual component *allocate(entityManager *manager,
		                            entity *ent,
		                            nlohmann::json source)
		{
			return new syncRigidBodyTransform(manager, ent);
		}
};

class syncRigidBodyPositionSerializer : public componentSerializer {
	public:
		constexpr static const char *serializedType = "syncRigidBodyPosition";

		virtual component *allocate(entityManager *manager, entity *ent) {
			return new syncRigidBodyPosition(manager, ent);
		}
};

class syncRigidBodyXZVelocitySerializer : public componentSerializer {
	public:
		constexpr static const char *serializedType = "syncRigidBodyXZVelocity";

		virtual component *allocate(entityManager *manager, entity *ent) {
			return new syncRigidBodyXZVelocity(manager, ent);
		}
};

class collisionHandlerSerializer : public componentSerializer {
	public:
		constexpr static const char *serializedType = "collisionHandler";

		virtual nlohmann::json serialize(component *comp) {
			collisionHandler *handler = dynamic_cast<collisionHandler*>(comp);

			if (handler) {
				return {{"tags", handler->tags}};

			} else {
				return {};
			}
		}

		virtual void deserialize(component *comp, nlohmann::json source) {
			collisionHandler *handler = dynamic_cast<collisionHandler*>(comp);

			if (handler && source.contains("tags") && source["tags"].is_array()) {
				for (auto& str : source["tags"].items()) {
					handler->tags.push_back(str.value().get<std::string>());
				}
			}
		}

		/*
		virtual component *allocate(entityManager *manager, entity *ent) {
			return new collisionHandler(manager, ent);
		}
		*/
};

// namespace grendx::ecs
}

