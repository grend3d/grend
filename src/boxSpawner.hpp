#pragma once

#include <grend/gameObject.hpp>
#include <grend/animation.hpp>
#include <grend/ecs/ecs.hpp>
#include <grend/ecs/rigidBody.hpp>

#include "inputHandler.hpp"
#include "projectile.hpp"

using namespace grendx;
using namespace grendx::ecs;

class boxBullet : public projectile {
	public:
		boxBullet(entityManager *manager, gameMain *game, glm::vec3 position);
};

class boxSpawner : public inputHandler {
	public:
		// TODO: could set firing rate, bullet amount, etc in properties
		boxSpawner(entityManager *manager,
		           entity *ent,
		           nlohmann::json properties={})
			: inputHandler(manager, ent)
		{
			manager->registerComponent(ent, "boxSpawner", this);
		}

		virtual void
		handleInput(entityManager *manager, entity *ent, inputEvent& ev);

		// serialization stuff
		constexpr static const char *serializedType = "boxSpawner";

		virtual const char *typeString(void) const { return serializedType; };
		virtual nlohmann::json serialize(entityManager *manager);
};
