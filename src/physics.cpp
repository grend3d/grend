#include <grend/physics.hpp>

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
	objects[ret].sphere.radius = r;
	objects[ret].position = pos;

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

void imp_physics::apply_force(uint64_t id, glm::vec3 force) {
	objects[id].velocity += force*(1/60.f);
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
	for (auto& [id, obj] : objects) {
		apply_force(id, {0, -9.81, 0});
	}

	for (unsigned i = 0; i < 5; i++) {
		const auto& collisions = find_collisions(delta);

		for (const auto& x : collisions) {
			auto& obj = objects[x.a];

			obj.velocity = glm::reflect(obj.velocity, x.normal)*0.5f;
			obj.position += x.normal*(float)static_geom.leaf_size*0.5f;
			//obj.position += x.normal*x.depth*(float)static_geom.leaf_size*0.5f;
			//obj.position += x.normal*x.depth*0.2f;
		}
	}

	for (auto& [id, obj] : objects) {
		obj.position += obj.velocity * delta;

		if (obj.position.y < -25) {
			// XXX: prevent objects from disappearing into the void
			obj.position = {0, 2, 0};
		}
	}
}
