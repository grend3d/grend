#include <grend/ecs/ecs.hpp>
#include <grend/ecs/rigidBody.hpp>

namespace grendx::ecs {

transformUpdatable::~transformUpdatable() {};
// key functions for rtti
rigidBody::~rigidBody() {
	//phys = nullptr;
	phys.reset();
};
rigidBodySphere::~rigidBodySphere() {};
rigidBodyBox::~rigidBodyBox() {};
rigidBodyCylinder::~rigidBodyCylinder() {};
rigidBodyCapsule::~rigidBodyCapsule() {};
rigidBodyStaticMesh::~rigidBodyStaticMesh() {};

syncRigidBody::~syncRigidBody() {};
syncRigidBodyTransform::~syncRigidBodyTransform() {};
syncRigidBodyPosition::~syncRigidBodyPosition() {};
syncRigidBodyXZVelocity::~syncRigidBodyXZVelocity() {};
syncRigidBodySystem::~syncRigidBodySystem() {};

rigidBody::rigidBody(regArgs t)
	: component(doRegister(this, t))
{
	manager->registerInterface<activatable>(t.ent, this);
	manager->registerInterface<transformUpdatable>(t.ent, this);
	registerCollisionQueue(manager->collisions);
}

rigidBody::rigidBody(regArgs t, float _mass)
	: rigidBody(std::move(t))
{
	mass = _mass;
}

rigidBodySphere::rigidBodySphere(regArgs t)
	: rigidBody(doRegister(this, t)) { }

rigidBodyBox::rigidBodyBox(regArgs t)
	: rigidBody(doRegister(this, t)) { }

rigidBodyCylinder::rigidBodyCylinder(regArgs t)
	: rigidBody(doRegister(this, t)) { }

rigidBodyCapsule::rigidBodyCapsule(regArgs t)
	: rigidBody(doRegister(this, t)) { }

rigidBodyStaticMesh::rigidBodyStaticMesh(regArgs t)
	: rigidBody(doRegister(this, t))
{
	t.manager->registerInterface<updatable>(t.ent, this);
	t.ent->messages.subscribe<sceneComponentAdded>(this->mbox);
	activate(t.manager, t.ent);
}

void syncRigidBodyTransform::sync(entityManager *manager, entity *ent) {
	rigidBody *body;
	castEntityComponent(body, manager, ent);

	if (!body || !body->phys) {
		// no attached physics body
		return;
	}

	ent->transform = body->phys->getTransform();
}

void syncRigidBodyPosition::sync(entityManager *manager, entity *ent) {
	rigidBody *body;
	castEntityComponent(body, manager, ent);

	if (!body || !body->phys) {
		// no attached physics body
		return;
	}

	ent->transform.position = body->phys->getTransform().position;

	/*
	TRS transform = ent->node->getTransformTRS();
	transform.position = body->phys->getTransform().position;
	ent->node->setTransform(transform);
	*/
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

	ent->transform.position = physTransform.position;
	ent->transform.rotation = rot;

	/*
	TRS transform = ent->node->getTransformTRS();
	transform.position = physTransform.position;
	transform.rotation = rot;
	ent->node->setTransform(transform);
	*/
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
	//auto& m = manager->getEntityComponents(ent);
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
