#include <grend/ecs/ecs.hpp>
#include <grend/ecs/rigidBody.hpp>

namespace grendx::ecs {

transformUpdatable::~transformUpdatable() {};
// key functions for rtti
rigidBody::~rigidBody() {
	//std::cerr << "got to ~rigidBody()" << std::endl;
	//phys = nullptr;
	phys.reset();
};
rigidBodySphere::~rigidBodySphere() {};
rigidBodyBox::~rigidBodyBox() {};
rigidBodyCylinder::~rigidBodyCylinder() {};
rigidBodyCapsule::~rigidBodyCapsule() {};
syncRigidBody::~syncRigidBody() {};
syncRigidBodyTransform::~syncRigidBodyTransform() {};
syncRigidBodyPosition::~syncRigidBodyPosition() {};
syncRigidBodyXZVelocity::~syncRigidBodyXZVelocity() {};
syncRigidBodySystem::~syncRigidBodySystem() {};

rigidBody::rigidBody(entityManager *manager, entity *ent)
	: component(manager, ent)
{
	manager->registerComponent(ent, this);
	manager->registerInterface<activatable>(ent, this);
	manager->registerInterface<transformUpdatable>(ent, this);
	registerCollisionQueue(manager->collisions);
}

rigidBody::rigidBody(entityManager *manager, entity *ent, float _mass)
	: rigidBody(manager, ent)
{
	mass = _mass;
}

rigidBodySphere::rigidBodySphere(entityManager *manager, entity *ent)
	: rigidBody(manager, ent)
{
	manager->registerComponent(ent, this);
	glm::vec3 position = ent->node->getTransformTRS().position;
	// incomplete initialization, needs properties set
	// TODO: warn somehow if the body is used before initialization

	/*
	phys = manager->engine->phys->addSphere(ent, position, 1.f, properties["radius"]);
	registerCollisionQueue(manager->collisions);
	*/
}

rigidBodyBox::rigidBodyBox(entityManager *manager, entity *ent)
	: rigidBody(manager, ent)
{
	nlohmann::json foo;
	manager->registerComponent(ent, this);

	// TODO: position is a common attribute, also duplicated between
	//       the entity transform, why not in the base rigidBody class,
	//       if here at all?
	glm::vec3 position = ent->node->getTransformTRS().position;
	// incomplete initialization, needs properties set

	/*
	auto center = properties["center"];
	auto extent = properties["extent"];

	AABBExtent box = {
		.center = glm::vec3(center[0], center[1], center[2]),
		.extent = glm::vec3(extent[0], extent[1], extent[2]),
	};

	phys = manager->engine->phys->addBox(ent, position, 1.f, box);
	registerCollisionQueue(manager->collisions);
	*/
}

rigidBodyCylinder::rigidBodyCylinder(entityManager *manager, entity *ent)
	: rigidBody(manager, ent)
{
	manager->registerComponent(ent, this);

	glm::vec3 position = ent->node->getTransformTRS().position;
	// incomplete initialization, needs properties set
	/*
	auto center = properties["center"];
	auto extent = properties["extent"];

	AABBExtent box = {
		.center = glm::vec3(center[0], center[1], center[2]),
		.extent = glm::vec3(extent[0], extent[1], extent[2]),
	};

	phys = manager->engine->phys->addCylinder(ent, position, 1.f, box);
	registerCollisionQueue(manager->collisions);
	*/
}

rigidBodyCapsule::rigidBodyCapsule(entityManager *manager, entity *ent)
	: rigidBody(manager, ent)
{
	manager->registerComponent(ent, this);

	glm::vec3 position = ent->node->getTransformTRS().position;
	/*
	float radius = properties["radius"];
	float height = properties["height"];

	phys = manager->engine->phys->addCapsule(ent, position, 1.f, radius, height);
	registerCollisionQueue(manager->collisions);
	*/
}

void syncRigidBodyTransform::sync(entityManager *manager, entity *ent) {
	rigidBody *body;
	castEntityComponent(body, manager, ent);

	if (!body || !body->phys) {
		// no attached physics body
		return;
	}

	ent->node->setTransform(body->phys->getTransform());
}

void syncRigidBodyPosition::sync(entityManager *manager, entity *ent) {
	rigidBody *body;
	castEntityComponent(body, manager, ent);

	if (!body || !body->phys) {
		// no attached physics body
		return;
	}

	TRS transform = ent->node->getTransformTRS();
	transform.position = body->phys->getTransform().position;
	ent->node->setTransform(transform);
}

void syncRigidBodyXZVelocity::sync(entityManager *manager, entity *ent) {
	rigidBody *body;
	castEntityComponent(body, manager, ent);

	// TODO: need way to avoid syncing if velocity is low to avoid
	//       glitchy movement, but still allow updating explicitly
	if (!body || !body->phys /*|| glm::length(body->phys->getVelocity()) < 0.1*/) {
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
	//std::set<component*> syncers = manager->getComponents("syncRigidBody");
	std::set<component*> syncers = manager->getComponents<syncRigidBody>();

	for (auto& comp : syncers) {
		syncRigidBody *syncer = static_cast<syncRigidBody*>(comp);
		entity *ent = manager->getEntity(comp);

		if (!syncer || !ent || !ent->active) {
			// TODO: warning
			continue;
		}

		syncer->sync(manager, ent);
	}
}

void updateEntityTransforms(entityManager *manager,
                            entity *ent,
                            const TRS& transform)
{
	auto& m = manager->getEntityComponents(ent);
	//auto updaters = m.equal_range("transformUpdatable");
	//auto updaters = m.equal_range(getTypeName<transformUpdatable>());
	auto updaters = ent->getAll<transformUpdatable>();

	for (auto it = updaters.first; it != updaters.second; it++) {
		auto& [_, comp] = *it;

		transformUpdatable *updater = dynamic_cast<transformUpdatable*>(comp);

		if (updater) {
			updater->setTransform(transform);
		}
	}
}

// namespace grendx::ecs
}
