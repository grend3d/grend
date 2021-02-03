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
		virtual ~enemySpawner();
		virtual void update(entityManager *manager, float delta);
};
