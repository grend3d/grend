#pragma once

#include <grend/gameObject.hpp>
#include <grend/animation.hpp>
#include <grend/ecs/ecs.hpp>
#include <grend/ecs/collision.hpp>
#include "health.hpp"

using namespace grendx;
using namespace grendx::ecs;

class enemyCollision : public collisionHandler {
	float damage;
	float lastCollision = 0;

	public:
		enemyCollision(entityManager *manager, entity *ent, float _damage = 15.f)
			: collisionHandler(manager, ent, {"enemy"})
		{
			damage = _damage;
			manager->registerComponent(ent, "enemyCollision", this);
		}

		virtual void
		onCollision(entityManager *manager, entity *ent,
		            entity *other, collision& col)
		{
			float ticks = SDL_GetTicks() / 1000.f;

			// only take damage once per second
			if (ticks - lastCollision < 0.25) {
				return;
			}

			std::cerr << "enemy collision!" << std::endl;
			lastCollision = ticks;
			health *entHealth;

			castEntityComponent(entHealth, manager, ent, "health");

			if (entHealth) {
				float x = entHealth->damage(damage);
				std::cerr << "current health: " << x << std::endl;

				if (x == 0.f) {
					manager->remove(ent);
				}
			}
		};
};
