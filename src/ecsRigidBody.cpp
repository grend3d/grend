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

rigidBodyUpdateSystem::~rigidBodyUpdateSystem() {};

rigidBody::rigidBody(regArgs t)
	: component(doRegister(this, t))
{
	manager->registerInterface<activatable>(t.ent, this);
	manager->registerInterface<transformUpdatable>(t.ent, this);
	// TODO: FIX
	//registerCollisionQueue(manager->collisions);
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

void rigidBodyUpdateSystem::update(entityManager *manager, float delta) {
	for (auto [ent, body] : manager->search<rigidBody>()) {
		TRS transform = body->phys->getTransform();
		transform.scale = ent->transform.getTRS().scale;
		ent->transform.set(transform);
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
