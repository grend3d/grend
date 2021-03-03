#pragma once

#include <grend/gameObject.hpp>
#include <grend/animation.hpp>
#include <grend/ecs/ecs.hpp>
#include <grend/ecs/collision.hpp>
#include "health.hpp"

using namespace grendx;
using namespace grendx::ecs;

class projectile : public entity {
	public:
		projectile(entityManager *manager, gameMain *game, glm::vec3 position);

		virtual ~projectile();
		virtual void update(entityManager *manager, float delta);

		// TODO:
		float impactDamage = 25.f;
};

class projectileCollision : public collisionHandler {
	public:
		projectileCollision(entityManager *manager, entity *ent)
			: collisionHandler(manager, ent, {"projectile"})
		{
			manager->registerComponent(ent, "projectileCollision", this);
		}

		projectileCollision(entityManager *manager,
		                    entity *ent,
		                    nlohmann::json properties);

		virtual ~projectileCollision();

		virtual void
		onCollision(entityManager *manager, entity *ent,
		            entity *other, collision& col)
		{
			std::cerr << "projectile collision!" << std::endl;
			health *entHealth;
			projectile *proj;

			castEntityComponent(entHealth, manager, ent, "health");
			castEntityComponent(proj, manager, other, "projectile");

			if (entHealth && proj) {
				float x = entHealth->damage(proj->impactDamage);
				std::cerr << "current health: " << x << std::endl;

				if (x == 0.f) {
					manager->remove(ent);
				}
			}
		};

		// serialization stuff
		constexpr static const char *serializedType = "player";

		virtual const char *typeString(void) const { return serializedType; };
		virtual nlohmann::json serialize(entityManager *manager); 
};

class projectileDestruct : public collisionHandler {
	public:
		projectileDestruct(entityManager *manager, entity *ent)
			: collisionHandler(manager, ent, {"projectileCollision"})
		{
			manager->registerComponent(ent, "projectileDestruct", this);
		}

		virtual ~projectileDestruct();

		virtual void
		onCollision(entityManager *manager, entity *ent,
		            entity *other, collision& col) {
			std::cerr << "projectile destruct!" << std::endl;
			manager->remove(ent);
		};
};

