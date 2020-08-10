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
				   .roughness = 0.9,
				   .metalness = 0.0,
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

renderer::renderer() {
	for (auto& [name, tex] : default_materials) {
		std::cerr << __func__ << ": loading materials for "
			<< name << std::endl;

		if (tex.diffuse_map.loaded()) {
			diffuse_handles[name] = gen_texture();
			diffuse_handles[name]->buffer(tex.diffuse_map, true);
		}

		if (tex.metal_roughness_map.loaded()) {
			specular_handles[name] = gen_texture();
			specular_handles[name]->buffer(tex.metal_roughness_map);
		}

		if (tex.normal_map.loaded()) {
			normmap_handles[name] = gen_texture();
			normmap_handles[name]->buffer(tex.normal_map);
		}

		if (tex.ambient_occ_map.loaded()) {
			aomap_handles[name] = gen_texture();
			aomap_handles[name]->buffer(tex.ambient_occ_map);
		}
	}

	reflection_atlas = std::unique_ptr<atlas>(new atlas(2048));
	shadow_atlas = std::unique_ptr<atlas>(new atlas(2048, atlas::mode::Depth));

	std::cerr << __func__ << ": Reached end of constructor" << std::endl;
}

void renderer::set_material(gl_manager::compiled_model& obj, std::string mat_name) {
	if (obj.materials.find(mat_name) == obj.materials.end()) {
		// TODO: maybe show a warning
		set_default_material(mat_name);
		//std::cerr << " ! couldn't find material " << mat_name << std::endl;
		return;
	}

	material& mat = obj.materials[mat_name];

	shader->set("anmaterial.diffuse", mat.diffuse);
	shader->set("anmaterial.ambient", mat.ambient);
	shader->set("anmaterial.specular", mat.specular);
	shader->set("anmaterial.roughness", mat.roughness);
	shader->set("anmaterial.metalness", mat.metalness);
	shader->set("anmaterial.opacity", mat.opacity);

	glActiveTexture(GL_TEXTURE0);
	if (obj.mat_textures.find(mat_name) != obj.mat_textures.end()) {
		obj.mat_textures[mat_name]->bind();
		shader->set("diffuse_map", 0);

	} else {
		diffuse_handles[fallback_material]->bind();
		shader->set("diffuse_map", 0);
	}

	glActiveTexture(GL_TEXTURE1);
	if (obj.mat_specular.find(mat_name) != obj.mat_specular.end()) {
		obj.mat_specular[mat_name]->bind();
		shader->set("specular_map", 1);

	} else {
		specular_handles[fallback_material]->bind();
		shader->set("specular_map", 1);
	}

	glActiveTexture(GL_TEXTURE2);
	if (obj.mat_normal.find(mat_name) != obj.mat_normal.end()) {
		obj.mat_normal[mat_name]->bind();
		shader->set("normal_map", 2);

	} else {
		normmap_handles[fallback_material]->bind();
		shader->set("normal_map", 2);
	}

	glActiveTexture(GL_TEXTURE3);
	if (obj.mat_ao.find(mat_name) != obj.mat_ao.end()) {
		obj.mat_ao[mat_name]->bind();
		shader->set("ambient_occ_map", 3);

	} else {
		aomap_handles[fallback_material]->bind();
		shader->set("ambient_occ_map", 3);
	}

	DO_ERROR_CHECK();
}

void renderer::set_default_material(std::string mat_name) {
	if (default_materials.find(mat_name) == default_materials.end()) {
		mat_name = fallback_material;
		puts("asdf");
	}

	material& mat = default_materials[mat_name];

	shader->set("anmaterial.diffuse", mat.diffuse);
	shader->set("anmaterial.ambient", mat.ambient);
	shader->set("anmaterial.specular", mat.specular);
	shader->set("anmaterial.roughness", mat.roughness);
	shader->set("anmaterial.metalness", mat.metalness);
	shader->set("anmaterial.opacity", mat.opacity);

	auto loaded_or_fallback = [&] (auto tex) {
		if (tex.loaded()) {
			return mat_name;
		} else {
			return fallback_material;
		}
	};

	glActiveTexture(GL_TEXTURE0);
	diffuse_handles[loaded_or_fallback(mat.diffuse_map)]->bind();
	shader->set("diffuse_map", 0);

	glActiveTexture(GL_TEXTURE1);
	specular_handles[loaded_or_fallback(mat.metal_roughness_map)]->bind();
	shader->set("specular_map", 1);

	glActiveTexture(GL_TEXTURE2);
	normmap_handles[loaded_or_fallback(mat.normal_map)]->bind();
	shader->set("normal_map", 2);

	glActiveTexture(GL_TEXTURE3);
	aomap_handles[loaded_or_fallback(mat.ambient_occ_map)]->bind();
	shader->set("ambient_occ_map", 3);
}

void renderer::set_mvp(glm::mat4 mod, glm::mat4 view, glm::mat4 projection) {
	glm::mat4 v_inv = glm::inverse(view);

	set_m(mod);
	shader->set("v", view);
	shader->set("p", projection);
	shader->set("v_inv", v_inv);
}

static glm::mat4 model_to_world(glm::mat4 model) {
	glm::quat rotation = glm::quat_cast(model);

	return glm::mat4_cast(rotation);
}

void renderer::set_m(glm::mat4 mod) {
	glm::mat3 m_3x3_inv_transp = glm::transpose(glm::inverse(model_to_world(mod)));

	shader->set("m", mod);
	shader->set("m_3x3_inv_transp", m_3x3_inv_transp);
}

void renderer::set_reflection_probe(const struct draw_attributes *attr) {
	glm::vec4 pos = attr->transform * glm::vec4(1);
	struct reflection_probe *probe = get_nearest_refprobe(glm::vec3(pos) / pos.w);

	if (probe) {
		for (unsigned i = 0; i < 6; i++) {
			std::string sloc = "reflection_probe[" + std::to_string(i) + "]";
			glm::vec3 facevec = reflection_atlas->tex_vector(probe->faces[i]); 
			shader->set(sloc, facevec);
			DO_ERROR_CHECK();
		}
	}
}

struct reflection_probe *renderer::get_nearest_refprobe(glm::vec3 pos) {
	struct reflection_probe *ret = nullptr;
	float mindist = 1024.0;

	// TODO: optimize this, O(N) is meh
	for (auto& [id, p] : ref_probes) {
		float dist = glm::distance(pos, p.position);
		if (!ret || dist < mindist) {
			mindist = dist;
			ret = &p;
		}
	}

	return ret;
}

void renderer::draw_mesh(std::string name, const struct draw_attributes *attr) {
	DO_ERROR_CHECK();
	gl_manager::compiled_mesh& foo = glman.cooked_meshes[name];
	set_m(attr->transform);

	// TODO: might be a performance issue, toggleable
	set_reflection_probe(attr);

	glman.set_face_order(attr->face_order);
	glman.bind_vao(foo.vao);
	glDrawElements(GL_TRIANGLES, foo.elements_size, GL_UNSIGNED_INT, foo.elements_offset);
	DO_ERROR_CHECK();
}

// TODO: overload of this that takes a material
void renderer::draw_mesh_lines(std::string name, const struct draw_attributes *attr) {
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

void renderer::draw_model(const struct draw_attributes *attr) {
	gl_manager::compiled_model& obj = glman.cooked_models[attr->name];

	for (std::string& name : obj.meshes) {
		set_material(obj, glman.cooked_meshes[name].material);
		draw_mesh(name, attr);
	}
}

void renderer::draw_model(struct draw_attributes attr) {
	draw_model(attr);
}

void renderer::draw_model_lines(const struct draw_attributes *attr) {
	gl_manager::compiled_model& obj = glman.cooked_models[attr->name];
	// TODO: need a set_material() function to handle stuff
	//       and we need to explicitly set a material

	set_default_material("(null)");
	for (std::string& name : obj.meshes) {
		draw_mesh_lines(name, attr);
	}
}

void renderer::draw_model_lines(struct draw_attributes attr) {
	draw_model_lines(&attr);
}

void renderer::draw_screenquad(void) {
	// NOTE: assumes that the screenquad vbo has been linked in the
	//       postprocessing shader, and the proper vao set
	glDrawArrays(GL_TRIANGLES, 0, 6);
	DO_ERROR_CHECK();
}

void
renderer::dqueue_draw_mesh(std::string mesh, const struct draw_attributes *attr) {
	draw_queue.push_back({mesh, *attr});
}

void renderer::dqueue_draw_model(const struct draw_attributes *attr) {
	// TODO: check that attr->name exists
	auto& obj = glman.cooked_models[attr->name];

	for (auto& str : obj.meshes) {
		dqueue_draw_mesh(str, attr);
	}
}

void renderer::dqueue_draw_model(struct draw_attributes attr) {
	dqueue_draw_model(&attr);
}

// TODO: should this just be called from dqueue_flush_draws?
void renderer::dqueue_sort_draws(glm::vec3 camera) {
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

void renderer::dqueue_cull_models(glm::vec3 camera) {
	// TODO: filter objects that aren't visible, behind the camera
	//       or occluded by walls etc
}

void renderer::dqueue_flush_draws(void) {
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

bool renderer::is_valid_light(uint32_t id) {
	//return id < MAX_LIGHTS && id < (int)active_lights && id >= 0;
	return id && id <= light_ids && (
		point_lights.find(id) != point_lights.end()
		|| spot_lights.find(id) != spot_lights.end()
		|| directional_lights.find(id) != directional_lights.end()
	);
}

uint32_t renderer::add_light(struct point_light lit) {
	uint32_t ret = alloc_light();
	point_lights[ret] = lit;

	for (unsigned i = 0; i < 6; i++) {
		point_lights[ret].shadowmap[i] = shadow_atlas->tree.alloc(256);
	}

	return ret;
}

uint32_t renderer::add_light(struct spot_light lit) {
	uint32_t ret = alloc_light();
	spot_lights[ret] = lit;
	spot_lights[ret].shadowmap = shadow_atlas->tree.alloc(128);
	return ret;
}

uint32_t renderer::add_light(struct directional_light lit) {
	uint32_t ret = alloc_light();
	directional_lights[ret] = lit;
	directional_lights[ret].shadowmap = shadow_atlas->tree.alloc(128);
	return ret;
}

void renderer::free_light(uint32_t id) {
	point_lights.erase(id);
	spot_lights.erase(id);
	directional_lights.erase(id);

	touch_light_shadowmaps();
}

struct point_light renderer::get_point_light(uint32_t id) {
	if (point_lights.find(id) != point_lights.end()) {
		return point_lights[id];
	}

	// return... something
	return (struct point_light){};
}

struct spot_light renderer::get_spot_light(uint32_t id) {
	if (spot_lights.find(id) != spot_lights.end()) {
		return spot_lights[id];
	}

	return (struct spot_light){};
}

struct directional_light renderer::get_directional_light(uint32_t id) {
	if (directional_lights.find(id) != directional_lights.end()) {
		return directional_lights[id];
	}

	return (struct directional_light){};
}

void renderer::set_point_light(uint32_t id, struct point_light *lit) {
	assert(lit != nullptr);
	point_lights[id] = *lit;
}

void renderer::set_spot_light(uint32_t id, struct spot_light *lit) {
	assert(lit != nullptr);
	spot_lights[id] = *lit;
}

void
renderer::set_directional_light(uint32_t id, struct directional_light *lit) {
	assert(lit != nullptr);
	directional_lights[id] = *lit;
}

uint32_t renderer::add_reflection_probe(struct reflection_probe ref) {
	uint32_t ret = refprobe_ids++;

	// TODO: configurable/deduced dimensions
	for (unsigned i = 0; i < 6; i++) {
		ref.faces[i] = reflection_atlas->tree.alloc(128);
	}

	ref_probes[ret] = ref;
	return ret;
}

void renderer::free_reflection_probe(uint32_t id) {
	if (ref_probes.find(id) != ref_probes.end()) {
		ref_probes.erase(id);
		// TODO: this is just here to test the texture cache, this should just
		//       free the node from the tree (O(log n)) rather than touching
		//       every node (O(n log n))
		touch_light_refprobes();
	}
}

void renderer::set_shader(Program::ptr prog) {
	if (shader != prog) {
		shader = prog;
		prog->bind();
	}
}

// TODO: only update changed uniforms
void renderer::sync_point_lights(const std::vector<uint32_t>& lights) {
	size_t active = min(MAX_LIGHTS, lights.size());

	shader->set("active_point_lights", (GLint)active);

	for (size_t i = 0; i < active; i++) {
		// TODO: check light index
		const auto& light = point_lights[lights[i]];

		// TODO: also updating all these uniforms can't be very efficient for a lot of
		//       moving point lights... maybe look into how uniform buffer objects work
		std::string locstr = "point_lights[" + std::to_string(i) + "]";

		shader->set(locstr + ".position",      light.position);
		shader->set(locstr + ".diffuse",       light.diffuse);
		shader->set(locstr + ".radius",        light.radius);
		shader->set(locstr + ".intensity",     light.intensity);
		shader->set(locstr + ".casts_shadows", light.casts_shadows);

		if (light.casts_shadows) {
			for (unsigned k = 0; k < 6; k++) {
				std::string sloc = locstr + ".shadowmap[" + std::to_string(k) + "]";
				shader->set(sloc, shadow_atlas->tex_vector(light.shadowmap[k]));
			}
		}
	}
}

void renderer::sync_spot_lights(const std::vector<uint32_t>& lights) {
	size_t active = min(MAX_LIGHTS, lights.size());

	shader->set("active_spot_lights", (GLint)active);

	for (size_t i = 0; i < active; i++) {
		// TODO: check light index
		const auto& light = spot_lights[lights[i]];
		std::string locstr = "spot_lights[" + std::to_string(i) + "]";

		shader->set(locstr + ".position",      light.position);
		shader->set(locstr + ".diffuse",       light.diffuse);
		shader->set(locstr + ".direction",     light.direction);
		shader->set(locstr + ".radius",        light.radius);
		shader->set(locstr + ".intensity",     light.intensity);
		shader->set(locstr + ".angle",         light.angle);
		shader->set(locstr + ".casts_shadows", light.casts_shadows);

		if (light.casts_shadows) {
			shader->set(locstr + ".shadowmap",
			            shadow_atlas->tex_vector(light.shadowmap));
		}
	}
}

void renderer::sync_directional_lights(const std::vector<uint32_t>& lights) {
	size_t active = min(MAX_LIGHTS, lights.size());

	shader->set("active_directional_lights", (GLint)active);

	for (size_t i = 0; i < active; i++) {
		// TODO: check light index
		const auto& light = directional_lights[lights[i]];
		std::string locstr = "directional_lights[" + std::to_string(i) + "]";

		shader->set(locstr + ".position",      light.position);
		shader->set(locstr + ".diffuse",       light.diffuse);
		shader->set(locstr + ".direction",     light.direction);
		shader->set(locstr + ".intensity",     light.intensity);
		shader->set(locstr + ".casts_shadows", light.casts_shadows);
	}
}

void renderer::compile_lights(void) {
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

void renderer::update_lights(void) {
	compile_lights();
	sync_point_lights(active_lights.point);
	sync_spot_lights(active_lights.spot);
	sync_directional_lights(active_lights.directional);
	DO_ERROR_CHECK();
}

void renderer::touch_light_refprobes(void) {
	for (auto& [id, probe] : ref_probes) {
		for (unsigned i = 0; i < 6; i++) {
			if (reflection_atlas->tree.valid(probe.faces[i])) {
				probe.faces[i] = reflection_atlas->tree.refresh(probe.faces[i]);
			}
		}
	}
}

// TODO: touch lights that are visible
void renderer::touch_light_shadowmaps(void) {
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

float grendx::light_extent(struct point_light *p, float threshold) {
	if (p) {
		//auto& lit = lights[id];
		return p->radius * (sqrt((p->intensity * p->diffuse.w)/threshold) - 1);
	}

	return 0.0;
}
