#include <grend/ecs/ecs.hpp>
#include <grend/ecs/rigidBody.hpp>

namespace grendx::ecs {

// key functions for rtti
rigidBody::~rigidBody() {
	std::cerr << "got to ~rigidBody()" << std::endl;
	//phys = nullptr;
	phys.reset();
};
rigidBodySphere::~rigidBodySphere() {};
rigidBodyBox::~rigidBodyBox() {};
syncRigidBody::~syncRigidBody() {};
syncRigidBodyTransform::~syncRigidBodyTransform() {};
syncRigidBodyPosition::~syncRigidBodyPosition() {};
syncRigidBodyXZVelocity::~syncRigidBodyXZVelocity() {};
syncRigidBodySystem::~syncRigidBodySystem() {};


rigidBody::rigidBody(entityManager *manager,
                     entity *ent,
                     nlohmann::json properties)
	: component(manager, ent, properties)
{
	manager->registerComponent(ent, serializedType, this);
}

rigidBodySphere::rigidBodySphere(entityManager *manager,
                                 entity *ent,
                                 nlohmann::json properties)
	: rigidBody(manager, ent, properties)
{
	manager->registerComponent(ent, serializedType, this);

	glm::vec3 position = ent->node->getTransformTRS().position;
	phys = manager->engine->phys->addSphere(ent, position, 1.f, properties["radius"]);
	registerCollisionQueue(manager->collisions);
}

rigidBodyBox::rigidBodyBox(entityManager *manager,
                           entity *ent,
                           nlohmann::json properties)
	: rigidBody(manager, ent, properties)
{
	// TODO
}

nlohmann::json rigidBodySphere::serialize(entityManager *manager) {
	// TODO: actually serialize
	return defaultProperties();
}

nlohmann::json rigidBodyBox::serialize(entityManager *manager) {
	// TODO: actually serialize
	return defaultProperties();
}

void syncRigidBodyTransform::sync(entityManager *manager, entity *ent) {
	rigidBody *body;
	castEntityComponent(body, manager, ent, "rigidBody");

	if (!body) {
		// no attached physics body
		return;
	}

	ent->node->setTransform(body->phys->getTransform());
}

void syncRigidBodyPosition::sync(entityManager *manager, entity *ent) {
	rigidBody *body;
	castEntityComponent(body, manager, ent, "rigidBody");

	if (!body) {
		// no attached physics body
		return;
	}

	TRS transform = ent->node->getTransformTRS();
	transform.position = body->phys->getTransform().position;
	ent->node->setTransform(transform);
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

	TRS transform = ent->node->getTransformTRS();
	transform.position = physTransform.position;
	transform.rotation = rot;
	ent->node->setTransform(transform);
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
