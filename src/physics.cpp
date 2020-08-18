#include <grend/physics.hpp>
#include <bullet/btBulletDynamicsCommon.h>

using namespace grendx;

// each return physics object ID
uint64_t imp_physics::add_static_model(std::string modname,
                                      grendx::model& mod,
                                      glm::mat4 transform)
{
	// XXX: for now, don't allocate an object for static meshes,
	//      although it may be a useful thing in the future
	uint64_t ret = 0;

	static_geom.add_model(mod, transform);

	return ret;
}

uint64_t imp_physics::add_sphere(std::string modname, glm::vec3 pos, double r)
{
	uint64_t ret = alloc_id();

	objects[ret].id = ret;
	objects[ret].model_name = modname;
	objects[ret].type = object::type::Sphere;
	objects[ret].usphere.radius = r;
	objects[ret].position = pos;
	// TODO: mass parameter
	objects[ret].inverse_mass = 1;

	return ret;
}

uint64_t imp_physics::add_box(std::string modname,
                              glm::vec3 pos,
                              double length,
                              double width,
                              double height)
{
	uint64_t ret = alloc_id();

	return ret;
}

/*
// map of submesh name to physics object ID
// TODO: multimap?
std::map<std::string, uint64_t>
grendx::add_model_mesh_boxes(model& mod) {
	std::map<std::string, uint64_t> ret;

	return ret;
}
*/

void imp_physics::remove(uint64_t id) {

}

void imp_physics::set_acceleration(uint64_t id, glm::vec3 accel) {
	objects[id].acceleration = accel;
}

std::list<imp_physics::collision> imp_physics::find_collisions(float delta) {
	std::list<imp_physics::collision> ret;

	for (auto& [id, obj] : objects) {
		glm::vec3 end = obj.position + obj.velocity*delta;
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

void imp_physics::solve_contraints(float delta) {
	/*
	for (auto& [id, obj] : objects) {
		apply_force(id, {0, -9.81, 0});
	}
	*/

	for (unsigned i = 0; i < 1; i++) {
		const auto& collisions = find_collisions(delta);

		for (const auto& x : collisions) {
			auto& obj = objects[x.a];

			obj.velocity = glm::reflect(obj.velocity, x.normal);
			//obj.position += x.normal*(float)static_geom.leaf_size*0.01f;
			obj.position += x.normal*x.depth;
			//obj.position += x.normal*x.depth*static_geom.leaf_size;
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
			obj.position = {0, 2, 0};
		}
	}
}
