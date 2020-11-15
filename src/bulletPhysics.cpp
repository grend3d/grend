#ifdef PHYSICS_BULLET
#include <grend/physics.hpp>
#include <grend/bulletPhysics.hpp>
//#include "btBulletDynamicsCommon.h"

using namespace grendx;

bulletPhysics::bulletPhysics() {
	collisionConfig = new btDefaultCollisionConfiguration();
	dispatcher      = new btCollisionDispatcher(collisionConfig);
	pairCache       = new btDbvtBroadphase();
	solver          = new btSequentialImpulseConstraintSolver();
	world           = new btDiscreteDynamicsWorld(dispatcher, pairCache,
	                                              solver, collisionConfig);

	world->setGravity(btVector3(0, -15, 0));
}

bulletPhysics::~bulletPhysics() {
	delete world;
	delete solver;
	delete pairCache;
	delete dispatcher;
	delete collisionConfig;
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
	body->setLinearVelocity(btVector3(vel.x, vel.y, vel.z));
}

void bulletObject::setAcceleration(glm::vec3 accel) {
	// TODO: how do you set acceleration?
	body->setLinearVelocity(btVector3(accel.x, accel.y, accel.z));
	//body->applyCentralImpulse(btVector3(accel.x, accel.y, accel.z));
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

// each return physics object ID
// non-moveable geometry, collisions with octree
physicsObject::ptr
bulletPhysics::addStaticModels(gameObject::ptr obj,
                               const TRS& transform)
{
	// XXX: for now, don't allocate an object for static meshes,
	//      although it may be a useful thing in the future
	physicsObject::ptr ret = nullptr;

	if (obj->type == gameObject::objType::Mesh) {
		if (auto p = obj->parent.lock()) {
			gameMesh::ptr mesh = std::dynamic_pointer_cast<gameMesh>(obj);
			gameModel::ptr model = std::dynamic_pointer_cast<gameModel>(p);

			if (mesh && model) {
				addStaticMesh(obj, transform, model, mesh);
			}
		}
	}

	for (auto& [name, node] : obj->nodes) {
		TRS adjTrans = addTRS(transform, node->transform);
		addStaticModels(node, adjTrans);
	}

	return ret;

}

// dynamic geometry, collisions with AABB tree
physicsObject::ptr
bulletPhysics::addSphere(gameObject::ptr obj, glm::vec3 pos,
                         float mass, float r)
{
	bulletObject::ptr ret = std::make_shared<bulletObject>();

	ret->mass = mass;
	ret->obj = obj;
	ret->shape = new btSphereShape(btScalar(r));

	bool isDynamic = mass != 0.f;
	btVector3 localInertia(0, 1, 0);
	btTransform trans;

	trans.setIdentity();
	trans.setOrigin(btVector3(pos.x, pos.y, pos.z));

	if (isDynamic) {
		ret->shape->calculateLocalInertia(btScalar(mass), localInertia);
	}

	// TODO: apparently motion state is optional? what does it provide?
	ret->motionState = new btDefaultMotionState(trans);
	ret->body = new btRigidBody(btScalar(mass), ret->motionState, ret->shape, localInertia);
	ret->body->setLinearFactor(btVector3(1, 1, 1));
	world->addRigidBody(ret->body);

	auto p = std::dynamic_pointer_cast<physicsObject>(ret);
	objects.insert(p);
	return p;
}

physicsObject::ptr
bulletPhysics::addBox(gameObject::ptr obj,
                      glm::vec3 position,
                      float mass,
                      AABBExtent& box)
{
	bulletObject::ptr ret = std::make_shared<bulletObject>();
	ret->mass = mass;
	ret->obj = obj;
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

	// TODO: apparently motion state is optional? what does it provide?
	ret->motionState = new btDefaultMotionState(trans);
	ret->body = new btRigidBody(btScalar(mass), ret->motionState, ret->shape, localInertia);
	ret->body->setLinearFactor(btVector3(1, 1, 1));
	ret->body->setAngularFactor(btScalar(1));
	world->addRigidBody(ret->body);

	auto p = std::dynamic_pointer_cast<physicsObject>(ret);
	objects.insert(p);
	return p;
}

physicsObject::ptr
bulletPhysics::addStaticMesh(gameObject::ptr obj,
                             const TRS& transform,
                             gameModel::ptr model,
                             gameMesh::ptr mesh)
{
	btTriangleIndexVertexArray* indexArray =
		new btTriangleIndexVertexArray(mesh->faces.size() / 3,
		                               (int*)mesh->faces.data(),
		                               sizeof(GLuint[3]),
		                               model->vertices.size(),
		                               (GLfloat*)model->vertices.data(),
									   sizeof(GLfloat[3]));

	bulletObject::ptr ret = std::make_shared<bulletObject>();
	ret->mass = 0.f;
	ret->obj = obj;
	ret->shape = new btBvhTriangleMeshShape(indexArray, true /* useQuantizedAabbCompression */ );
	//ret->shape->setMargin(0.05);

	btVector3 scale(transform.scale.x, transform.scale.y, transform.scale.z);
	ret->shape->setLocalScaling(scale);

	//bool isDynamic = mass != 0.f;
	btVector3 localInertia(0, 0, 0);
	btTransform trans;

	//glm::vec3 pos = position + box.center;
	glm::vec3 pos = transform.position;
	glm::quat rot = transform.rotation;
	trans.setIdentity();
	trans.setOrigin(btVector3(pos.x, pos.y, pos.z));
	trans.setRotation(btQuaternion(rot.x, rot.y, rot.z, rot.w));

	/*
	if (isDynamic) {
		ret->shape->calculateLocalInertia(btScalar(mass), localInertia);
	}
	*/

	// TODO: apparently motion state is optional? what does it provide?
	ret->motionState = new btDefaultMotionState(trans);
	ret->body = new btRigidBody(btScalar(0.f), ret->motionState, ret->shape, localInertia);
	ret->body->setLinearFactor(btVector3(1, 1, 1));
	ret->body->setAngularFactor(btScalar(1));
	world->addRigidBody(ret->body);

	auto p = std::dynamic_pointer_cast<physicsObject>(ret);
	objects.insert(p);
	return p;
}

// map of submesh name to physics object ID
// TODO: multimap?
std::map<gameMesh::ptr, physicsObject::ptr>
bulletPhysics::addModelMeshBoxes(gameModel::ptr mod) {
	return {};
}

void bulletPhysics::remove(physicsObject::ptr obj) {

}

void bulletPhysics::clear(void) {

}

size_t bulletPhysics::numObjects(void) {
	return objects.size();
}

std::list<physics::collision> bulletPhysics::findCollisions(float delta) {
	return {};
}

void bulletPhysics::stepSimulation(float delta) {
	world->stepSimulation(1.f/ 60.f, 10);

	for (auto& k : objects) {
		bulletObject::ptr ptr = std::dynamic_pointer_cast<bulletObject>(k);
		btTransform trans;

		if (ptr && ptr->body && ptr->body->getMotionState()) {
			ptr->body->getMotionState()->getWorldTransform(trans);

		} else {
			trans = ptr->body->getWorldTransform();
		}

		if (ptr->mass != 0) {
			ptr->obj->transform = ptr->getTransform();
		}
	}
}
#endif
