#include <grend/physics.hpp>
#include <grend/impPhysics.hpp>

// TODO: physics abstract class with derived implementations
//#include <bullet/btBulletDynamicsCommon.h>

using namespace grendx;

void impObject::setTransform(TRS& transform) {
	position = transform.position;
	rotation = transform.rotation;
}

TRS impObject::getTransform(void) {
	return (TRS) {};
}

void impObject::setPosition(glm::vec3 pos) {
	//objects[id].position = pos;
	position = pos;
}

void impObject::setVelocity(glm::vec3 vel) {
	//objects[id].velocity = vel;
	velocity = vel;
}

glm::vec3 impObject::getVelocity(void) {
	return velocity;
}

void impObject::setAcceleration(glm::vec3 accel) {
	acceleration = accel;
}

void impObject::setAngularFactor(float amount) {
	// TODO: implement impObject::setAngularFactor();
}

glm::vec3 impObject::getAcceleration(void) {
	return acceleration;
}

physicsObject::ptr
impPhysics::addStaticModels(void *data,
                            sceneNode::ptr obj,
                            glm::mat4 transform)
{
	glm::mat4 adjTrans = transform*obj->getTransform(0);

	// XXX: for now, don't allocate an object for static meshes,
	//      although it may be a useful thing in the future
	physicsObject::ptr ret = 0;

	if (obj->type == sceneNode::objType::Model) {
		sceneModel::ptr model = std::dynamic_pointer_cast<sceneModel>(obj);
		static_geom.add_model(model, adjTrans);
	}

	for (auto& [name, node] : obj->nodes) {
		addStaticModels(data, node, adjTrans);
	}

	return ret;
}

physicsObject::ptr
impPhysics::addSphere(void *data,
                      glm::vec3 pos,
                      float mass,
                      float r)
{
	impObject::ptr impobj = std::make_shared<impObject>();

	impobj->data = data;
	impobj->type = impObject::type::Sphere;
	impobj->usphere.radius = r;
	impobj->position = pos;
	impobj->inverseMass = mass;

	auto pobj = std::dynamic_pointer_cast<physicsObject>(impobj);
	objects.insert(pobj);

	return pobj;
}

physicsObject::ptr
impPhysics::addBox(void *data,
                   glm::vec3 position,
                   float mass,
                   AABBExtent& box)
{
	impObject::ptr ret = std::make_shared<impObject>();
	// TODO: add_box()

	return std::dynamic_pointer_cast<physicsObject>(ret);
}

// TODO: implement add_model_mesh_boxes()
std::map<sceneMesh::ptr, physicsObject::ptr>
impPhysics::addModelMeshBoxes(sceneModel::ptr mod) {
	return {};
}

void impPhysics::remove(physicsObject::ptr obj) {
	objects.erase(obj);
}

void impPhysics::clear(void) {

}

size_t impPhysics::numObjects(void) {
	return objects.size();
}

void impPhysics::filterCollisions(void) {
	for (auto& pobj : objects) {
		impObject::ptr obj = std::dynamic_pointer_cast<impObject>(pobj);

		glm::vec3 end =
			obj->position
			+ obj->velocity
			// XXX: radius for sphere collisions only
			+ glm::normalize(obj->velocity)*obj->usphere.radius*2.f;

		auto [depth, normal] = static_geom.collides(obj->position, end);

		if (depth > 0) {
			// TODO: call collision handler
			/*
			({
				obj, nullptr,
				obj->position,
				normal,
				depth,
			});
			*/
		}
	}
}

std::list<collision> impPhysics::findCollisions(float delta) {
	std::list<collision> ret;

	for (auto& pobj : objects) {
		impObject::ptr obj = std::dynamic_pointer_cast<impObject>(pobj);

		glm::vec3 end =
			obj->position
			+ obj->velocity*delta
			// XXX: radius for sphere collisions only
			+ glm::normalize(obj->velocity)*obj->usphere.radius*2.f;

		auto [depth, normal] = static_geom.collides(obj->position, end);

		if (depth > 0) {
			// TODO:
			/*
			ret.push_back({
				obj, nullptr,
				obj->position,
				normal,
				depth,
			});
			*/
		}
	}

	return ret;
}

void impPhysics::stepSimulation(float delta) {
	if (delta < 0 || std::isnan(delta) || std::isinf(delta)) {
		// invalid delta, just return
		return;
	}

	for (unsigned i = 0; i < 1; i++) {
		const auto& collisions = findCollisions(delta);

		for (const auto& x : collisions) {
			auto& pobj = x.a;
			impObject::ptr obj = std::dynamic_pointer_cast<impObject>(pobj);

			obj->velocity = glm::reflect(obj->velocity, x.normal);
			obj->position += x.normal*x.depth;
		}
	}

	for (auto& pobj : objects) {
		impObject::ptr obj = std::dynamic_pointer_cast<impObject>(pobj);
		float d = pow(obj->drag_s, delta);

		//obj.position += obj.velocity * delta;
		obj->position += obj->velocity*delta + obj->acceleration*((delta*delta)/2);
		obj->velocity += obj->acceleration*delta + glm::vec3(0, -15, 0)*delta;
		obj->velocity *= powf(0.5, delta);

		if (obj->position.y < -25) {
			// XXX: prevent objects from disappearing into the void
			//obj.position = {0, 10, 0};
			obj->position.y = 10;
		}

		///obj->obj->transform.position = obj->position;
	}
}
