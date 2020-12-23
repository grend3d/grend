#include <grend/ecs/ecs.hpp>
#include <grend/ecs/collision.hpp>

namespace grendx::ecs {

void entitySystemCollision::update(entityManager *manager, float delta) {
	for (auto& col : *manager->collisions) {
		if (col.adata && col.bdata) {
			entity *ents[2] = {
				static_cast<entity*>(col.adata),
				static_cast<entity*>(col.bdata),
			};

			for (unsigned i = 0; i < 2; i++) {
				entity *self  = ents[i];
				entity *other = ents[!i];

				if (manager->hasComponents(self, {"collisionHandler", "entity"})) {
					collisionHandler *collider =
						castEntityComponent<collisionHandler*>(manager, self, "collisionHandler");

					if (!collider) {
						// no collision component...?
						continue;
					}

					if (manager->hasComponents(other, collider->tags)) {
						collider->onCollision(manager, self, col);
					}
				}
			}
		}
	}
}

// namespace grendx::ecs
}
