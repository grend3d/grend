#pragma once

#include <grend/gameObject.hpp>
#include <grend/animation.hpp>
#include <grend/modalSDLInput.hpp>

#include <grend/ecs/ecs.hpp>
#include <grend/ecs/rigidBody.hpp>
#include <grend/ecs/collision.hpp>

using namespace grendx;
using namespace grendx::ecs;

class enemy : public entity {
	public:
		enemy(entityManager *manager, gameMain *game, glm::vec3 position);
		enemy(entityManager *manager, entity *ent, nlohmann::json properties);

		virtual ~enemy();
		virtual void update(entityManager *manager, float delta);
		virtual gameObject::ptr getNode(void) { return node; };

		// serialization stuff
		constexpr static const char *serializedType = "enemy";
		static const nlohmann::json defaultProperties(void) {
			return entity::defaultProperties();
		}

		virtual const char *typeString(void) const { return serializedType; };
		virtual nlohmann::json serialize(entityManager *manager); 
};

