#pragma once

#include <grend/gameObject.hpp>
#include <grend/animation.hpp>
#include <grend/modalSDLInput.hpp>

#include <grend/ecs/ecs.hpp>
#include <grend/ecs/rigidBody.hpp>
#include <grend/ecs/collision.hpp>

using namespace grendx;
using namespace grendx::ecs;

class enemySpawner : public entity {
	float lastSpawn = 0;

	public:
		enemySpawner(entityManager *manager, gameMain *game, glm::vec3 position);
		enemySpawner(entityManager *manager, entity *ent, nlohmann::json properties);
		virtual ~enemySpawner();
		virtual void update(entityManager *manager, float delta);

		// serialization stuff
		constexpr static const char *serializedType = "enemySpawner";
		static const nlohmann::json defaultProperties(void) {
			return entity::defaultProperties();
		}

		virtual const char *typeString(void) const { return serializedType; };
		virtual nlohmann::json serialize(entityManager *manager); 
};
