#pragma once

#include <grend/gameObject.hpp>
#include <grend/animation.hpp>
#include <grend/ecs/ecs.hpp>
#include <grend/ecs/collision.hpp>

using namespace grendx;
using namespace grendx::ecs;

class projectile : public entity {
	public:
		projectile(entityManager *manager, gameMain *game, glm::vec3 position);
		virtual void update(entityManager *manager, float delta);
};

class projectileCollision : public collisionHandler {
	public:
		projectileCollision(entityManager *manager, entity *ent)
			: collisionHandler(manager, ent, {"projectile"})
		{
			manager->registerComponent(ent, "projectileCollision", this);
		}

		virtual void
		onCollision(entityManager *manager, entity *ent, collision& col) {
			std::cerr << "projectile collision!" << std::endl;
			if (false /* check health or something */) {
				manager->remove(ent);
			}
		};
};

class projectileDestruct : public collisionHandler {
	public:
		projectileDestruct(entityManager *manager, entity *ent)
			: collisionHandler(manager, ent, {"projectileCollision"})
		{
			manager->registerComponent(ent, "projectileDestruct", this);
		}

		virtual void
		onCollision(entityManager *manager, entity *ent, collision& col) {
			std::cerr << "projectile destruct!" << std::endl;
			manager->remove(ent);
		};
};

