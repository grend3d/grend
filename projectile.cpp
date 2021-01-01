#include <grend/ecs/rigidBody.hpp>
#include "projectile.hpp"
#include "timedLifetime.hpp"

projectile::projectile(entityManager *manager, gameMain *game, glm::vec3 position)
	: entity(manager)
{
	manager->registerComponent(this, "projectile", this);

	// TODO: configurable projectile attributes
	new rigidBodySphere(manager, this, position, 1.0, 0.15);
	new projectileDestruct(manager, this);
	new timedLifetime(manager, this);

	node->transform.position = position;
}

void projectile::update(entityManager *manager, float delta) {
	rigidBody *body;
	castEntityComponent(body, manager, this, "rigidBody");

	if (body) {
		body->syncPhysics(this);
	}
}

