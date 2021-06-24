#include <grend-config.h>

#ifdef PHYSICS_BULLET
#include <grend/physics.hpp>
#include <grend/bulletPhysics.hpp>
//#include "btBulletDynamicsCommon.h"

using namespace grendx;

// non-pure virtual destructors for rtti
// TODO: move physics* objects to own source file
physics::~physics() {};

physicsObject::~physicsObject() {
	std::cerr << "BBBBBBB : removing physics object!!!!!! " << std::endl;
};
bulletObject::~bulletObject() {
	std::cerr << "AAAAAAA : removing bullet object!!!!!! " << std::endl;
	this->removeSelf();
}

bulletPhysics::bulletPhysics() {
	collisionConfig = new btDefaultCollisionConfiguration();
	dispatcher      = new btCollisionDispatcher(collisionConfig);
	pairCache       = new btDbvtBroadphase();
	solver          = new btSequentialImpulseConstraintSolver();
	world           = new btDiscreteDynamicsWorld(dispatcher, pairCache,
	                                              solver, collisionConfig);

	world->setGravity(btVector3(0, -15, 0));
	world->setDebugDrawer(&debugDrawer);
}

bulletPhysics::~bulletPhysics() {
	delete world;
	delete solver;
	delete pairCache;
	delete dispatcher;
	delete collisionConfig;
}

#include <iostream>

void bulletObject::removeSelf(void) {
	std::cerr << "AAAAAAAAAAAAAAAAAA: got here! removing bullet object" << std::endl;
	if (runtime) {
		std::cerr << "removing bullet object 0x" << std::hex << this << std::endl;
		runtime->remove(this);
	}
}

void bulletObject::setTransform(TRS& transform) {

}

TRS bulletObject::getTransform(void) {
	btTransform trans;

	if (body && body->getMotionState()) {
		body->getMotionState()->getWorldTransform(trans);

	} else {
		trans = body->getWorldTransform();
	}

	glm::vec3 position = glm::vec3(
		float(trans.getOrigin().getX()),
		float(trans.getOrigin().getY()),
		float(trans.getOrigin().getZ())
	);

	btQuaternionFloatData bdata;
	trans.getRotation().serializeFloat(bdata);
	glm::quat rotation = glm::quat(
		// glm quats are wxyz, btquaternion xyzw
		bdata.m_floats[3], bdata.m_floats[0],
		bdata.m_floats[1], bdata.m_floats[2]
	);

	return {
		.position = position,
		.rotation = rotation,
	};
}

void bulletObject::setPosition(glm::vec3 pos) {
}

void bulletObject::setVelocity(glm::vec3 vel) {
	body->activate(true);
	body->setLinearVelocity(btVector3(vel.x, vel.y, vel.z));
}

void bulletObject::setAcceleration(glm::vec3 accel) {
	body->activate(true);
	body->applyCentralForce(btVector3(accel.x, accel.y, accel.z));
}

void bulletObject::setAngularFactor(float amount) {
	body->setAngularFactor(btScalar(amount));
	// TODO: implement impObject::setAngularFactor();
}

glm::vec3 bulletObject::getVelocity(void) {
	// TODO
	auto v = body->getLinearVelocity();
	return glm::vec3(v.x(), v.y(), v.z());
}

glm::vec3 bulletObject::getAcceleration(void) {
	// TODO
	return glm::vec3(0);
}

void bulletPhysics::drawDebug(glm::mat4 cam) {
	if (debugDrawer.getDebugMode()) {
		world->debugDrawWorld();
		debugDrawer.flushLines(cam);
	}
}

void bulletPhysics::setDebugMode(int mode) {
	debugDrawer.setDebugMode(mode);
}

void
bulletPhysics::addStaticModels(void *data,
                               gameObject::ptr obj,
                               const TRS& transform,
                               std::vector<physicsObject::ptr>& collector)
{
	// XXX: for now, don't allocate an object for static meshes,
	//      although it may be a useful thing in the future
	physicsObject::ptr ret = nullptr;

	if (obj->type == gameObject::objType::Mesh) {
		if (auto p = obj->parent.lock()) {
			gameMesh::ptr mesh = std::dynamic_pointer_cast<gameMesh>(obj);
			gameModel::ptr model = std::dynamic_pointer_cast<gameModel>(p);

			if (mesh && model) {
				collector.push_back(addStaticMesh(data, transform, model, mesh));
			}
		}
	}

	for (auto& [name, node] : obj->nodes) {
		TRS adjTrans = addTRS(transform, node->getTransformTRS());
		addStaticModels(data, node, adjTrans, collector);
	}
}

// dynamic geometry, collisions with AABB tree
physicsObject::ptr
bulletPhysics::addSphere(void *data, glm::vec3 pos,
                         float mass, float r)
{
	std::lock_guard<std::mutex> lock(bulletMutex);

	bulletObject::ptr ret = std::make_shared<bulletObject>();
	ret->runtime = this;
	ret->mass = mass;
	ret->data = data;
	ret->shape = new btSphereShape(btScalar(r));

	bool isDynamic = mass != 0.f;
	btVector3 localInertia(0, 1, 0);
	btTransform trans;

	trans.setIdentity();
	trans.setOrigin(btVector3(pos.x, pos.y, pos.z));

	if (isDynamic) {
		ret->shape->calculateLocalInertia(btScalar(mass), localInertia);
	}

	ret->motionState = new btDefaultMotionState(trans);
	ret->body = new btRigidBody(btScalar(mass), ret->motionState, ret->shape, localInertia);
	ret->body->setLinearFactor(btVector3(1, 1, 1));
	ret->body->setUserPointer(ret.get());
	world->addRigidBody(ret->body);

	auto p = std::dynamic_pointer_cast<physicsObject>(ret);
	objects.insert({ret.get(), p});
	return p;
}

physicsObject::ptr
bulletPhysics::addBox(void *data,
                      glm::vec3 position,
                      float mass,
                      AABBExtent& box)
{
	std::lock_guard<std::mutex> lock(bulletMutex);

	bulletObject::ptr ret = std::make_shared<bulletObject>();
	ret->runtime = this;
	ret->mass = mass;
	ret->data = data;
	ret->shape = new btBoxShape(btVector3(box.extent.x, box.extent.y, box.extent.z));

	bool isDynamic = mass != 0.f;
	btVector3 localInertia(0, 0, 0);
	btTransform trans;

	glm::vec3 pos = position + box.center;
	trans.setIdentity();
	trans.setOrigin(btVector3(pos.x, pos.y, pos.z));

	if (isDynamic) {
		ret->shape->calculateLocalInertia(btScalar(mass), localInertia);
	}

	ret->motionState = new btDefaultMotionState(trans);
	ret->body = new btRigidBody(btScalar(mass), ret->motionState, ret->shape, localInertia);
	ret->body->setLinearFactor(btVector3(1, 1, 1));
	ret->body->setAngularFactor(btScalar(1));
	ret->body->setUserPointer(ret.get());
	world->addRigidBody(ret->body);

	auto p = std::dynamic_pointer_cast<physicsObject>(ret);
	//obj->physObj = p;
	objects.insert({ret.get(), p});
	return p;
}

physicsObject::ptr
bulletPhysics::addCylinder(void *data,
                           glm::vec3 position,
                           float mass,
                           AABBExtent& box)
{
	std::lock_guard<std::mutex> lock(bulletMutex);

	bulletObject::ptr ret = std::make_shared<bulletObject>();
	ret->runtime = this;
	ret->mass = mass;
	ret->data = data;
	ret->shape = new btCylinderShape(btVector3(box.extent.x, box.extent.y, box.extent.z));

	bool isDynamic = mass != 0.f;
	btVector3 localInertia(0, 0, 0);
	btTransform trans;

	glm::vec3 pos = position + box.center;
	trans.setIdentity();
	trans.setOrigin(btVector3(pos.x, pos.y, pos.z));

	if (isDynamic) {
		ret->shape->calculateLocalInertia(btScalar(mass), localInertia);
	}

	ret->motionState = new btDefaultMotionState(trans);
	ret->body = new btRigidBody(btScalar(mass), ret->motionState, ret->shape, localInertia);
	ret->body->setLinearFactor(btVector3(1, 1, 1));
	ret->body->setAngularFactor(btScalar(1));
	ret->body->setUserPointer(ret.get());
	world->addRigidBody(ret->body);

	auto p = std::dynamic_pointer_cast<physicsObject>(ret);
	//obj->physObj = p;
	objects.insert({ret.get(), p});
	return p;
}

physicsObject::ptr
bulletPhysics::addCapsule(void *data,
                          glm::vec3 position,
                          float mass,
                          float radius,
                          float height)
{
	std::lock_guard<std::mutex> lock(bulletMutex);

	bulletObject::ptr ret = std::make_shared<bulletObject>();
	ret->runtime = this;
	ret->mass = mass;
	ret->data = data;
	ret->shape = new btCapsuleShape(btScalar(radius), btScalar(height));

	bool isDynamic = mass != 0.f;
	btVector3 localInertia(0, 0, 0);
	btTransform trans;

	glm::vec3 pos = position;
	trans.setIdentity();
	trans.setOrigin(btVector3(pos.x, pos.y, pos.z));

	if (isDynamic) {
		ret->shape->calculateLocalInertia(btScalar(mass), localInertia);
	}

	ret->motionState = new btDefaultMotionState(trans);
	ret->body = new btRigidBody(btScalar(mass), ret->motionState, ret->shape, localInertia);
	ret->body->setLinearFactor(btVector3(1, 1, 1));
	ret->body->setAngularFactor(btScalar(1));
	ret->body->setUserPointer(ret.get());
	world->addRigidBody(ret->body);

	auto p = std::dynamic_pointer_cast<physicsObject>(ret);
	//obj->physObj = p;
	objects.insert({ret.get(), p});
	return p;
}

physicsObject::ptr
bulletPhysics::addStaticMesh(void *data,
                             const TRS& transform,
                             gameModel::ptr model,
                             gameMesh::ptr mesh)
{
	std::lock_guard<std::mutex> lock(bulletMutex);

	if (mesh->faces.size() / 3 == 0) {
		std::cerr << "WARNING: bulletPhysics::addStaticMesh(): "
			<< "can't add mesh with no elements" << std::endl;
		return nullptr;
	}

	btTriangleIndexVertexArray* indexArray =
		new btTriangleIndexVertexArray(mesh->faces.size() / 3,
		                               (int*)mesh->faces.data(),
		                               sizeof(GLuint[3]),
		                               model->vertices.size(),
		                               (GLfloat*)model->vertices.data(),
									   sizeof(gameModel::vertex));

	bulletObject::ptr ret = std::make_shared<bulletObject>();
	ret->runtime = this;
	ret->mass = 0.f;
	ret->data = data;
	ret->shape = new btBvhTriangleMeshShape(indexArray, true /* useQuantizedAabbCompression */ );

	btVector3 scale(transform.scale.x, transform.scale.y, transform.scale.z);
	ret->shape->setLocalScaling(scale);

	btVector3 localInertia(0, 0, 0);
	btTransform trans;

	glm::vec3 pos = transform.position;
	glm::quat rot = transform.rotation;
	trans.setIdentity();
	trans.setOrigin(btVector3(pos.x, pos.y, pos.z));
	trans.setRotation(btQuaternion(rot.x, rot.y, rot.z, rot.w));

	ret->motionState = new btDefaultMotionState(trans);
	ret->body = new btRigidBody(btScalar(0.f), ret->motionState, ret->shape, localInertia);
	ret->body->setLinearFactor(btVector3(1, 1, 1));
	ret->body->setAngularFactor(btScalar(1));
	ret->body->setUserPointer(ret.get());
	world->addRigidBody(ret->body);

	auto p = std::dynamic_pointer_cast<physicsObject>(ret);
	objects.insert({ret.get(), p});
	return p;
}

// TODO: remove, don't think this is being used anywhere, don't remember what I
//       had planned to do with it...
//       must not be important then :P
std::map<gameMesh::ptr, physicsObject::ptr>
bulletPhysics::addModelMeshBoxes(gameModel::ptr mod) {
	return {};
}

void bulletPhysics::remove(physicsObject::ptr obj) {
	std::lock_guard<std::mutex> lock(bulletMutex);

	bulletObject::ptr bobj = std::dynamic_pointer_cast<bulletObject>(obj);
	std::cerr << "remove(): got here, removing an object" << std::endl;

	if (bobj) {
		std::cerr << "remove(): really removing" << std::endl;
		remove(bobj.get());
	}
}

void bulletPhysics::remove(bulletObject *ptr) {
	std::lock_guard<std::mutex> lock(bulletMutex);

	auto it = objects.find(ptr);

	// XXX: linear search here is terrible, need more efficient solution
	if (it != objects.end()) {
		world->removeRigidBody(ptr->body);
		objects.erase(it);
	}
}

void bulletPhysics::clear(void) {
}

size_t bulletPhysics::numObjects(void) {
	return objects.size();
}

void bulletPhysics::filterCollisions(void) {
	int manifolds = world->getDispatcher()->getNumManifolds();

	for (int i = 0; i < manifolds; i++) {
		btPersistentManifold *contact =
			world->getDispatcher()->getManifoldByIndexInternal(i);
		const btCollisionObject *a = contact->getBody0();
		const btCollisionObject *b = contact->getBody1();

		bulletObject *aptr = static_cast<bulletObject*>(a->getUserPointer());
		bulletObject *bptr = static_cast<bulletObject*>(b->getUserPointer());

		if (!objects.count(aptr) || !objects.count(bptr)) {
			// one of the objects isn't valid, this shouldn't happen
			continue;
		}

		int numContacts = contact->getNumContacts();
		for (int k = 0; k < numContacts; k++) {
			btManifoldPoint& pt = contact->getContactPoint(k);
			float depth = pt.getDistance();

			if (depth < 0.f) {
				physicsObject::weakptr wphysA = objects[aptr];
				physicsObject::weakptr wphysB = objects[bptr];

				physicsObject::ptr physA = wphysA.lock();
				physicsObject::ptr physB = wphysB.lock();

				if (!physA || !physB) {
					// pointer to expired object, which is somehow still in the
					// world, shouldn't reach here but have to handle this anyway
					continue;
				}

				const btVector3& ptA = pt.getPositionWorldOnA();
				const btVector3& ptB = pt.getPositionWorldOnB();
				const btVector3& normalB =  pt.m_normalWorldOnB;
				const btVector3& normalA = -pt.m_normalWorldOnB;

				if (physA->collisionQueue) {
					collision colA = {
						.a = physA,
						.b = physB,
						.adata = aptr->data,
						.bdata = bptr->data,
						.position = glm::vec3(ptA.getX(), ptA.getY(), ptA.getZ()),
						.normal = glm::vec3(normalA.getX(), normalA.getY(), normalA.getZ()),
						.depth = depth,
					};

					physA->collisionQueue->push_back(colA);
				}

				if (physB->collisionQueue) {
					collision colB = {
						.a = physB,
						.b = physA,
						.adata = bptr->data,
						.bdata = aptr->data,
						.position = glm::vec3(ptB.getX(), ptB.getY(), ptB.getZ()),
						.normal = glm::vec3(normalB.getX(), normalB.getY(), normalB.getZ()),
						.depth = depth,
					};

					physB->collisionQueue->push_back(colB);
				}
			}
		}
	}
}

void bulletPhysics::stepSimulation(float delta) {
	std::lock_guard<std::mutex> lock(bulletMutex);
	world->stepSimulation(delta, 10);

	for (auto& [rawptr, wphysptr] : objects) {
		physicsObject::ptr physptr = wphysptr.lock();

		if (!physptr) {
			continue;
		}

		bulletObject::ptr ptr = std::dynamic_pointer_cast<bulletObject>(physptr);
		btTransform trans;

		if (ptr && ptr->body && ptr->body->getMotionState()) {
			ptr->body->getMotionState()->getWorldTransform(trans);

		} else {
			trans = ptr->body->getWorldTransform();
		}

		if (ptr->mass != 0) {
			if (auto ref = ptr->obj.lock()) {
				//ref->transform = ptr->getTransform();
				ref->setTransform(ptr->getTransform());
			}
		}
	}
}
#endif
