#include <grend/engine.hpp>
#include <grend/model.hpp>
#include <grend/utility.hpp>

#include <vector>
#include <map>
#include <iostream>
#include <algorithm>

using namespace grendx;

static std::map<std::string, material> default_materials = {
	{"(null)", {
				   .diffuse = {0.75, 0.75, 0.75, 1},
				   .ambient = {1, 1, 1, 1},
				   .specular = {0.5, 0.5, 0.5, 1},
				   .roughness = 0.5,
				   .metalness = 0.5,
				   .opacity = 1,
				   .refract_idx = 1.5,

				   .diffuse_map     = material_texture("assets/tex/white.png"),
				   .metal_roughness_map    = material_texture("assets/tex/white.png"),
				   .normal_map      = material_texture("assets/tex/lightblue-normal.png"),
				   .ambient_occ_map = material_texture("assets/tex/white.png"),
			   }},

	{"Black",  {
				   .diffuse = {1, 0.8, 0.2, 1},
				   .ambient = {1, 0.8, 0.2, 1},
				   .specular = {1, 1, 1, 1},
				   .roughness = 0.5,
				   .metalness = 0.5,
				   .opacity = 1,
				   .refract_idx = 1.5,
			   }},

	{"Grey",   {
				   .diffuse = {0.1, 0.1, 0.1, 0.5},
				   .ambient = {0.1, 0.1, 0.1, 0.2},
				   .specular = {0.0, 0.0, 0.0, 0.05},
				   .roughness = 0.75,
				   .metalness = 0.5,
				   .opacity = 1,
				   .refract_idx = 1.5,
			   }},

	{"Yellow", {
				   .diffuse = {0.01, 0.01, 0.01, 1},
				   .ambient = {0, 0, 0, 1},
				   .specular = {0.2, 0.2, 0.2, 0.2},
				   .roughness = 0.3,
				   .metalness = 0.5,
				   .opacity = 1,
				   .refract_idx = 1.5,
			   }},

	{"Steel",  {
				   .diffuse = {1, 1, 1, 1},
				   .ambient = {1, 1, 1, 1},
				   .specular = {1, 1, 1, 1},
				   .roughness = 0.2,
				   .metalness = 0.5,
				   .opacity = 1,
				   .refract_idx = 1.5,

				   /*
				   .diffuse_map  = "assets/tex/rubberduck-tex/199.JPG",
				   .specular_map = "assets/tex/white.png",
				   .normal_map   = "assets/tex/rubberduck-tex/199_norm.JPG",
				   .ambient_occ_map = "assets/tex/white.png",
				   */

				   .diffuse_map  = material_texture("assets/tex/iron-rusted4-Unreal-Engine/iron-rusted4-basecolor.png"),
				   .metal_roughness_map = material_texture("assets/tex/iron-rusted4-Unreal-Engine/iron-rusted4-metalness.png"),
				   .normal_map   = material_texture("assets/tex/iron-rusted4-Unreal-Engine/iron-rusted4-normal.png"),
			   }},

	{"Wood",  {
				   .diffuse = {1, 1, 1, 1},
				   .ambient = {1, 1, 1, 1},
				   .specular = {1, 1, 1, 0.3},
				   .roughness = 0.7,
				   .metalness = 0.5,
				   .opacity = 1,
				   .refract_idx = 1.5,

				   .diffuse_map = material_texture("assets/tex/white.png"),
				   //.diffuse_map  = "assets/tex/rubberduck-tex/199.JPG",
				   //.diffuse_map  = material_texture("assets/tex/dims/Textures/Boards.JPG"),
				   .metal_roughness_map = material_texture("assets/tex/white.png"),
				   //.normal_map   = material_texture("assets/tex/dims/Textures/Textures_N/Boards_N.jpg"),
				   .normal_map = material_texture("assets/tex/white.png"),
				   //.normal_map   = "assets/tex/rubberduck-tex/199_norm.JPG",
				   .ambient_occ_map = material_texture("assets/tex/white.png"),
			   }},

	{"Glass",  {
				   //.diffuse = {0.2, 0.5, 0.2, 0},
				   .diffuse = {0.5, 0.5, 0.5, 0},
				   .ambient = {1, 1, 1, 1},
				   .specular = {1, 1, 1, 0.5},
				   .roughness = 0.05,
				   .metalness = 0.5,
				   .opacity = 0.1,
				   .refract_idx = 1.5,
			   }},

	{"Earth",  {
				   //.diffuse = {0.2, 0.5, 0.2, 0},
				   .diffuse = {1, 1, 1, 1},
				   .ambient = {1, 1, 1, 1},
				   .specular = {1, 1, 1, 0.5},
				   .roughness = 0.5,
				   .metalness = 0.5,
				   .opacity = 1.0,
				   .refract_idx = 1.5,

				   //.diffuse_map  = material_texture("assets/tex/Earthmap720x360_grid.jpg"),
				   .diffuse_map = material_texture("assets/tex/white.png"),
			   }},
};

engine::engine() {
	for (auto& thing : default_materials) {
		std::cerr << __func__ << ": loading materials for "
			<< thing.first << std::endl;

		if (thing.second.diffuse_map.loaded()) {
			diffuse_handles[thing.first] =
				glman.buffer_texture(thing.second.diffuse_map, true /* srgb */);
		}

		if (thing.second.metal_roughness_map.loaded()) {
			specular_handles[thing.first] =
				glman.buffer_texture(thing.second.metal_roughness_map);
		}

		if (thing.second.normal_map.loaded()) {
			normmap_handles[thing.first] =
				glman.buffer_texture(thing.second.normal_map);
		}

		if (thing.second.ambient_occ_map.loaded()) {
			aomap_handles[thing.first] =
				glman.buffer_texture(thing.second.ambient_occ_map);
		}
	}

	reflection_atlas = std::unique_ptr<atlas>(new atlas(glman, 2048));
	shadow_atlas = std::unique_ptr<atlas>(new atlas(glman, 2048, atlas::mode::Depth));

	std::cerr << __func__ << ": Reached end of constructor" << std::endl;
}

void engine::set_material(gl_manager::compiled_model& obj, std::string mat_name) {
	if (obj.materials.find(mat_name) == obj.materials.end()) {
		// TODO: maybe show a warning
		set_default_material(mat_name);
		//std::cerr << " ! couldn't find material " << mat_name << std::endl;
		return;
	}

	material& mat = obj.materials[mat_name];
	auto shader_obj = glman.get_shader_obj(shader);

	shader_obj.set("anmaterial.diffuse", mat.diffuse);
	shader_obj.set("anmaterial.ambient", mat.ambient);
	shader_obj.set("anmaterial.specular", mat.specular);
	shader_obj.set("anmaterial.roughness", mat.roughness);
	shader_obj.set("anmaterial.metalness", mat.metalness);
	shader_obj.set("anmaterial.opacity", mat.opacity);

	glActiveTexture(GL_TEXTURE0);
	if (obj.mat_textures.find(mat_name) != obj.mat_textures.end()) {
		glBindTexture(GL_TEXTURE_2D, obj.mat_textures[mat_name].first);
		shader_obj.set("diffuse_map", 0);

	} else {
		glBindTexture(GL_TEXTURE_2D, diffuse_handles[fallback_material].first);
		shader_obj.set("diffuse_map", 0);
	}

	glActiveTexture(GL_TEXTURE1);
	if (obj.mat_specular.find(mat_name) != obj.mat_specular.end()) {
		// TODO: specular maps
		glBindTexture(GL_TEXTURE_2D, obj.mat_specular[mat_name].first);
		shader_obj.set("specular_map", 1);

	} else {
		glBindTexture(GL_TEXTURE_2D, specular_handles[fallback_material].first);
		shader_obj.set("specular_map", 1);
	}

	glActiveTexture(GL_TEXTURE2);
	if (obj.mat_normal.find(mat_name) != obj.mat_normal.end()) {
		glBindTexture(GL_TEXTURE_2D, obj.mat_normal[mat_name].first);
		shader_obj.set("normal_map", 2);

	} else {
		glBindTexture(GL_TEXTURE_2D, normmap_handles[fallback_material].first);
		shader_obj.set("normal_map", 2);
	}

	glActiveTexture(GL_TEXTURE3);
	if (obj.mat_ao.find(mat_name) != obj.mat_ao.end()) {
		glBindTexture(GL_TEXTURE_2D, obj.mat_ao[mat_name].first);
		shader_obj.set("ambient_occ_map", 3);

	} else {
		glBindTexture(GL_TEXTURE_2D, aomap_handles[fallback_material].first);
		shader_obj.set("ambient_occ_map", 3);
	}

	DO_ERROR_CHECK();
}

void engine::set_default_material(std::string mat_name) {
	if (default_materials.find(mat_name) == default_materials.end()) {
		// TODO: really show an error here
		mat_name = fallback_material;
		puts("asdf");
	}

	material& mat = default_materials[mat_name];
	auto shader_obj = glman.get_shader_obj(shader);

	shader_obj.set("anmaterial.diffuse", mat.diffuse);
	shader_obj.set("anmaterial.ambient", mat.ambient);
	shader_obj.set("anmaterial.specular", mat.specular);
	shader_obj.set("anmaterial.roughness", mat.roughness);
	shader_obj.set("anmaterial.metalness", mat.metalness);
	shader_obj.set("anmaterial.opacity", mat.opacity);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, diffuse_handles[mat.diffuse_map.loaded()?
	                                             mat_name
	                                             : fallback_material].first);
	shader_obj.set("diffuse_map", 0);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, specular_handles[mat.metal_roughness_map.loaded()?
	                                              mat_name
	                                              : fallback_material].first);
	shader_obj.set("specular_map", 1);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, normmap_handles[mat.normal_map.loaded()?
	                                             mat_name
	                                             : fallback_material].first);
	shader_obj.set("normal_map", 2);

	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, aomap_handles[mat.ambient_occ_map.loaded()?
	                                           mat_name
	                                           : fallback_material].first);
	shader_obj.set("ambient_occ_map", 3);
}

void engine::set_mvp(glm::mat4 mod, glm::mat4 view, glm::mat4 projection) {
	glm::mat4 v_inv = glm::inverse(view);
	auto shader_obj = glman.get_shader_obj(shader);

	set_m(mod);
	shader_obj.set("v", view);
	shader_obj.set("p", projection);
	shader_obj.set("v_inv", v_inv);
}

static glm::mat4 model_to_world(glm::mat4 model) {
	glm::quat rotation = glm::quat_cast(model);
	//rotation = -glm::conjugate(rotation);

	return glm::mat4_cast(rotation);
}

void engine::set_m(glm::mat4 mod) {
	glm::mat3 m_3x3_inv_transp = glm::transpose(glm::inverse(model_to_world(mod)));
	auto shader_obj = glman.get_shader_obj(shader);

	shader_obj.set("m", mod);
	shader_obj.set("m_3x3_inv_transp", m_3x3_inv_transp);
}

void engine::draw_mesh(std::string name, const struct draw_attributes *attr) {
	DO_ERROR_CHECK();
	gl_manager::compiled_mesh& foo = glman.cooked_meshes[name];
	set_m(attr->transform);

	glman.set_face_order(attr->face_order);
	glman.bind_vao(foo.vao);
	glDrawElements(GL_TRIANGLES, foo.elements_size, GL_UNSIGNED_INT, foo.elements_offset);
	DO_ERROR_CHECK();
}

// TODO: overload of this that takes a material
void engine::draw_mesh_lines(std::string name, const struct draw_attributes *attr) {
	gl_manager::compiled_mesh& foo = glman.cooked_meshes[name];

	set_m(attr->transform);
	glman.set_face_order(attr->face_order);
	glman.bind_vao(foo.vao);

#ifdef NO_GLPOLYMODE
	glDrawElements(GL_LINE_LOOP, foo.elements_size, GL_UNSIGNED_INT, foo.elements_offset);

#else
	// TODO: keep track of face culling state in this class
	// TODO: actually, keep a cache of enabled flags in the gl_manager class
#ifdef ENABLE_FACE_CULLING
	glman.disable(GL_CULL_FACE);
#endif

	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glDrawElements(GL_TRIANGLES, foo.elements_size, GL_UNSIGNED_INT, foo.elements_offset);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

#ifdef ENABLE_FACE_CULLING
	glman.enable(GL_CULL_FACE);
#endif
#endif
}

void engine::draw_model(const struct draw_attributes *attr) {
	gl_manager::compiled_model& obj = glman.cooked_models[attr->name];

	for (std::string& name : obj.meshes) {
		set_material(obj, glman.cooked_meshes[name].material);
		draw_mesh(name, attr);
	}
}

void engine::draw_model(struct draw_attributes attr) {
	draw_model(attr);
}

void engine::draw_model_lines(const struct draw_attributes *attr) {
	gl_manager::compiled_model& obj = glman.cooked_models[attr->name];
	// TODO: need a set_material() function to handle stuff
	//       and we need to explicitly set a material

	set_default_material("(null)");
	for (std::string& name : obj.meshes) {
		draw_mesh_lines(name, attr);
	}
}

void engine::draw_model_lines(struct draw_attributes attr) {
	draw_model_lines(&attr);
}

void engine::draw_screenquad(void) {
	// NOTE: assumes that the screenquad vbo has been linked in the
	//       postprocessing shader, and the proper vao set
	glDrawArrays(GL_TRIANGLES, 0, 6);
	DO_ERROR_CHECK();
}

void
engine::dqueue_draw_mesh(std::string mesh, const struct draw_attributes *attr) {
	draw_queue.push_back({mesh, *attr});
}

void engine::dqueue_draw_model(const struct draw_attributes *attr) {
	// TODO: check that attr->name exists
	auto& obj = glman.cooked_models[attr->name];

	for (auto& str : obj.meshes) {
		dqueue_draw_mesh(str, attr);
	}
}

void engine::dqueue_draw_model(struct draw_attributes attr) {
	dqueue_draw_model(&attr);
}

// TODO: should this just be called from dqueue_flush_draws?
void engine::dqueue_sort_draws(glm::vec3 camera) {
	typedef std::pair<std::string, struct draw_attributes> draw_ent;

	// sort by position from camera
	std::sort(draw_queue.begin(), draw_queue.end(),
		[&] (draw_ent a, draw_ent b) {
			glm::vec4 ta = a.second.transform * glm::vec4(1);
			glm::vec4 tb = b.second.transform * glm::vec4(1);
			glm::vec3 va = glm::vec3(ta) / ta.w;
			glm::vec3 vb = glm::vec3(tb) / tb.w;

			return glm::distance(camera, va) < glm::distance(camera, vb);
		});

	// split opaque and transparent objects into seperate lists
	for (auto it = draw_queue.begin(); it != draw_queue.end(); it++) {
		auto& obj = glman.cooked_models[it->second.name];
		auto& meshmat = glman.cooked_meshes[it->first].material;

		if (obj.materials.find(meshmat) != obj.materials.end()) {
			material& mat = obj.materials[meshmat];

			if (mat.blend != material::blend_mode::Opaque) {
				transparent_draws.push_back(*it);
				it = draw_queue.erase(it);
			}
		}
	}

	// these remain front-to-back sorted, so reversing makes it back-to-front
	// TODO: this makes dqueue_sort_draws() stateful, if it's called a second
	//       time the transparent draws will be in the wrong order, better way
	//       to do this without a redundant sort?
	std::reverse(transparent_draws.begin(), transparent_draws.end());
}

void engine::dqueue_cull_models(glm::vec3 camera) {
	// TODO: filter objects that aren't visible, behind the camera
	//       or occluded by walls etc
}

void engine::dqueue_flush_draws(void) {
	glman.disable(GL_BLEND);

	for (auto& [name, attrs] : draw_queue) {
		auto& obj = glman.cooked_models[attrs.name];

		set_material(obj, glman.cooked_meshes[name].material);
		draw_mesh(name, &attrs);
	}

	glman.enable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	for (auto& [name, attrs] : transparent_draws) {
		auto& obj = glman.cooked_models[attrs.name];

		set_material(obj, glman.cooked_meshes[name].material);
		draw_mesh(name, &attrs);
	}

	draw_queue.clear();
	transparent_draws.clear();
}

bool engine::is_valid_light(uint32_t id) {
	//return id < MAX_LIGHTS && id < (int)active_lights && id >= 0;
	return id && id <= light_ids && (
		point_lights.find(id) != point_lights.end()
		|| spot_lights.find(id) != spot_lights.end()
		|| directional_lights.find(id) != directional_lights.end()
	);
}

uint32_t engine::add_light(struct point_light lit) {
	uint32_t ret = alloc_light();
	point_lights[ret] = lit;

	for (unsigned i = 0; i < 6; i++) {
		point_lights[ret].shadowmap[i] = shadow_atlas->tree.alloc(256);
	}

	return ret;
}

uint32_t engine::add_light(struct spot_light lit) {
	uint32_t ret = alloc_light();
	spot_lights[ret] = lit;
	spot_lights[ret].shadowmap = shadow_atlas->tree.alloc(128);
	return ret;
}

uint32_t engine::add_light(struct directional_light lit) {
	uint32_t ret = alloc_light();
	directional_lights[ret] = lit;
	directional_lights[ret].shadowmap = shadow_atlas->tree.alloc(128);
	return ret;
}

void engine::free_light(uint32_t id) {
	point_lights.erase(id);
	spot_lights.erase(id);
	directional_lights.erase(id);

	touch_light_shadowmaps();
}

struct engine::point_light engine::get_point_light(uint32_t id) {
	if (point_lights.find(id) != point_lights.end()) {
		return point_lights[id];
	}

	// return... something
	return (struct point_light){};
}

struct engine::spot_light engine::get_spot_light(uint32_t id) {
	if (spot_lights.find(id) != spot_lights.end()) {
		return spot_lights[id];
	}

	return (struct spot_light){};
}

struct engine::directional_light engine::get_directional_light(uint32_t id) {
	if (directional_lights.find(id) != directional_lights.end()) {
		return directional_lights[id];
	}

	return (struct directional_light){};
}

void engine::set_point_light(uint32_t id, struct engine::point_light *lit) {
	assert(lit != nullptr);
	point_lights[id] = *lit;
}

void engine::set_spot_light(uint32_t id, struct engine::spot_light *lit) {
	assert(lit != nullptr);
	spot_lights[id] = *lit;
}

void
engine::set_directional_light(uint32_t id, struct engine::directional_light *lit) {
	assert(lit != nullptr);
	directional_lights[id] = *lit;
}

uint32_t engine::add_reflection_probe(struct reflection_probe ref) {
	uint32_t ret = refprobe_ids++;

	// TODO: configurable/deduced dimensions
	for (unsigned i = 0; i < 6; i++) {
		ref.faces[i] = reflection_atlas->tree.alloc(128);
	}

	ref_probes[ret] = ref;
	return ret;
}

void engine::free_reflection_probe(uint32_t id) {
	if (ref_probes.find(id) != ref_probes.end()) {
		ref_probes.erase(id);
		// TODO: this is just here to test the texture cache, this should just
		//       free the node from the tree (O(log n)) rather than touching
		//       every node (O(n log n))
		touch_light_refprobes();
	}
}

void engine::set_shader(gl_manager::rhandle& shd) {
	if (shader != shd) {
		shader = shd;
		glUseProgram(shader.first);
	}
}

// TODO: only update changed uniforms
void engine::sync_point_lights(const std::vector<uint32_t>& lights) {
	auto shader_obj = glman.get_shader_obj(shader);
	size_t active = min(MAX_LIGHTS, lights.size());

	shader_obj.set("active_point_lights", (GLint)active);

	for (size_t i = 0; i < active; i++) {
		// TODO: check light index
		const auto& light = point_lights[lights[i]];

		// TODO: also updating all these uniforms can't be very efficient for a lot of
		//       moving point lights... maybe look into how uniform buffer objects work
		std::string locstr = "point_lights[" + std::to_string(i) + "]";

		shader_obj.set(locstr + ".position",      light.position);
		shader_obj.set(locstr + ".diffuse",       light.diffuse);
		shader_obj.set(locstr + ".radius",        light.radius);
		shader_obj.set(locstr + ".intensity",     light.intensity);
		shader_obj.set(locstr + ".casts_shadows", light.casts_shadows);

		if (light.casts_shadows) {
			for (unsigned k = 0; k < 6; k++) {
				std::string sloc = locstr + ".shadowmap[" + std::to_string(k) + "]";
				shader_obj.set(sloc, shadow_atlas->tex_vector(light.shadowmap[k]));
			}
		}
	}
}

void engine::sync_spot_lights(const std::vector<uint32_t>& lights) {
	auto shader_obj = glman.get_shader_obj(shader);
	size_t active = min(MAX_LIGHTS, lights.size());

	shader_obj.set("active_spot_lights", (GLint)active);

	for (size_t i = 0; i < active; i++) {
		// TODO: check light index
		const auto& light = spot_lights[lights[i]];
		std::string locstr = "spot_lights[" + std::to_string(i) + "]";

		shader_obj.set(locstr + ".position",      light.position);
		shader_obj.set(locstr + ".diffuse",       light.diffuse);
		shader_obj.set(locstr + ".direction",     light.direction);
		shader_obj.set(locstr + ".radius",        light.radius);
		shader_obj.set(locstr + ".intensity",     light.intensity);
		shader_obj.set(locstr + ".angle",         light.angle);
		shader_obj.set(locstr + ".casts_shadows", light.casts_shadows);

		if (light.casts_shadows) {
			shader_obj.set(locstr + ".shadowmap",
			               shadow_atlas->tex_vector(light.shadowmap));
		}
	}
}

void engine::sync_directional_lights(const std::vector<uint32_t>& lights) {
	auto shader_obj = glman.get_shader_obj(shader);
	size_t active = min(MAX_LIGHTS, lights.size());

	shader_obj.set("active_directional_lights", (GLint)active);

	for (size_t i = 0; i < active; i++) {
		// TODO: check light index
		const auto& light = directional_lights[lights[i]];
		std::string locstr = "directional_lights[" + std::to_string(i) + "]";

		shader_obj.set(locstr + ".position",      light.position);
		shader_obj.set(locstr + ".diffuse",       light.diffuse);
		shader_obj.set(locstr + ".direction",     light.direction);
		shader_obj.set(locstr + ".intensity",     light.intensity);
		shader_obj.set(locstr + ".casts_shadows", light.casts_shadows);
	}
}

void engine::compile_lights(void) {
	active_lights.point.clear();
	active_lights.spot.clear();
	active_lights.directional.clear();

	for (auto& [id, lit] : point_lights) {
		active_lights.point.push_back(id);
	}

	for (auto& [id, lit] : spot_lights) {
		active_lights.spot.push_back(id);
	}

	for (auto& [id, lit] : directional_lights) {
		active_lights.directional.push_back(id);
	}
}

void engine::update_lights(void) {
	compile_lights();
	sync_point_lights(active_lights.point);
	sync_spot_lights(active_lights.spot);
	sync_directional_lights(active_lights.directional);
	DO_ERROR_CHECK();
}

void engine::touch_light_refprobes(void) {
	for (auto& [id, probe] : ref_probes) {
		for (unsigned i = 0; i < 6; i++) {
			if (reflection_atlas->tree.valid(probe.faces[i])) {
				probe.faces[i] = reflection_atlas->tree.refresh(probe.faces[i]);
			}
		}
	}
}

// TODO: touch lights that are visible
void engine::touch_light_shadowmaps(void) {
	for (auto& [id, plit] : point_lights) {
		if (!plit.casts_shadows)
			continue;

		for (unsigned i = 0; i < 6; i++) {
			if (reflection_atlas->tree.valid(plit.shadowmap[i])) {
				plit.shadowmap[i] =
					reflection_atlas->tree.refresh(plit.shadowmap[i]);
			}
		}
	}

	for (auto& [id, slit] : spot_lights) {
		if (!slit.casts_shadows)
			continue;

		if (reflection_atlas->tree.valid(slit.shadowmap)) {
			slit.shadowmap = reflection_atlas->tree.refresh(slit.shadowmap);
		}
	}
}

float grendx::light_extent(struct engine::point_light *p, float threshold) {
	if (p) {
		//auto& lit = lights[id];
		return p->radius * (sqrt((p->intensity * p->diffuse.w)/threshold) - 1);
	}

	return 0.0;
}
