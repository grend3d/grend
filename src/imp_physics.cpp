#include <grend/physics.hpp>
#include <grend/imp_physics.hpp>

// TODO: physics abstract class with derived implementations
//#include <bullet/btBulletDynamicsCommon.h>

using namespace grendx;

// each return physics object ID
uint64_t imp_physics::add_static_models(gameObject::ptr obj,
                                        glm::mat4 transform)
{
	glm::mat4 adjTrans = transform*obj->getTransform(0);

	// XXX: for now, don't allocate an object for static meshes,
	//      although it may be a useful thing in the future
	uint64_t ret = 0;

	if (obj->type == gameObject::objType::Model) {
		gameModel::ptr model = std::dynamic_pointer_cast<gameModel>(obj);
		static_geom.add_model(model, adjTrans);
	}

	for (auto& [name, node] : obj->nodes) {
		add_static_models(node, adjTrans);
	}

	return ret;
}

uint64_t imp_physics::add_sphere(gameObject::ptr obj,
                                 glm::vec3 pos,
                                 float mass,
                                 float r)
{
	uint64_t ret = alloc_id();

	objects[ret].obj = obj;
	objects[ret].id = ret;
	objects[ret].type = object::type::Sphere;
	objects[ret].usphere.radius = r;
	objects[ret].position = pos;
	// TODO: mass parameter
	objects[ret].inverse_mass = mass;

	return ret;
}

uint64_t imp_physics::add_box(gameObject::ptr obj,
                              glm::vec3 pos,
                              float mass,
                              float length,
                              float width,
                              float height)
{
	uint64_t ret = alloc_id();
	// TODO: add_box()

	return ret;
}

// TODO: implement add_model_mesh_boxes()
std::map<gameMesh::ptr, uint64_t>
imp_physics::add_model_mesh_boxes(gameModel::ptr mod) {
	return {};
}

void imp_physics::remove(uint64_t id) {

}

void imp_physics::clear(void) {

}

void imp_physics::set_acceleration(uint64_t id, glm::vec3 accel) {
	objects[id].acceleration = accel;
}

std::list<imp_physics::collision> imp_physics::find_collisions(float delta) {
	std::list<imp_physics::collision> ret;

	for (auto& [id, obj] : objects) {
		glm::vec3 end =
			obj.position
			+ obj.velocity*delta
			// XXX: radius for sphere collisions only
			+ glm::normalize(obj.velocity)*obj.usphere.radius*2.f;

		auto [depth, normal] = static_geom.collides(obj.position, end);

		if (depth > 0) {
			ret.push_back({
				id, 0,
				obj.position,
				normal,
				depth,
			});
		}
	}

	return ret;
}

void imp_physics::step_simulation(float delta) {
	if (delta < 0 || std::isnan(delta) || std::isinf(delta)) {
		// invalid delta, just return
		return;
	}

	for (unsigned i = 0; i < 1; i++) {
		const auto& collisions = find_collisions(delta);

		for (const auto& x : collisions) {
			auto& obj = objects[x.a];

			obj.velocity = glm::reflect(obj.velocity, x.normal);
			obj.position += x.normal*x.depth;
		}
	}

	for (auto& [id, obj] : objects) {
		float d = pow(obj.drag_s, delta);

		//obj.position += obj.velocity * delta;
		obj.position += obj.velocity*delta + obj.acceleration*((delta*delta)/2);
		obj.velocity += obj.acceleration*delta + glm::vec3(0, -15, 0)*delta;
		obj.velocity *= powf(0.5, delta);

		if (obj.position.y < -25) {
			// XXX: prevent objects from disappearing into the void
			//obj.position = {0, 10, 0};
			obj.position.y = 10;
		}

		obj.obj->transform.position = obj.position;
	}
}
