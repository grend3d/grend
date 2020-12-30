#include <grend/ecs/ecs.hpp>
#include <grend/ecs/rigidBody.hpp>

namespace grendx::ecs {

void syncRigidBodyTransform::sync(entityManager *manager, entity *ent) {
	rigidBody *body;
	castEntityComponent(body, manager, ent, "rigidBody");

	if (!body) {
		// no attached physics body
		return;
	}

	ent->node->transform = body->phys->getTransform();
}

void syncRigidBodyPosition::sync(entityManager *manager, entity *ent) {
	rigidBody *body;
	castEntityComponent(body, manager, ent, "rigidBody");

	if (!body) {
		// no attached physics body
		return;
	}

	ent->node->transform.position = body->phys->getTransform().position;
}

void syncRigidBodyXZVelocity::sync(entityManager *manager, entity *ent) {
	rigidBody *body;
	castEntityComponent(body, manager, ent, "rigidBody");

	if (!body) {
		// no attached physics body
		return;
	}

	TRS physTransform = body->phys->getTransform();
	glm::vec3 vel = body->phys->getVelocity();
	glm::quat rot = glm::quat(glm::vec3(0, atan2(vel.x, vel.z), 0));

	ent->node->transform.position = physTransform.position;
	ent->node->transform.rotation = rot;
}

void syncRigidBodySystem::update(entityManager *manager, float delta) {
	std::set<component*> syncers = manager->getComponents("syncRigidBody");

	for (auto& comp : syncers) {
		syncRigidBody *syncer = dynamic_cast<syncRigidBody*>(comp);
		entity *ent = manager->getEntity(comp);

		if (!syncer || !ent) {
			// TODO: warning
			continue;
		}

		syncer->sync(manager, ent);
	}
}

// namespace grendx::ecs
}
