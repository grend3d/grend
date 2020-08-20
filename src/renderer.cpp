#include <grend/engine.hpp>
#include <grend/gameModel.hpp>
#include <grend/utility.hpp>

#include <vector>
#include <map>
#include <iostream>
#include <algorithm>

using namespace grendx;

material default_material = {
	.diffuse = {0.75, 0.75, 0.75, 1},
	.ambient = {1, 1, 1, 1},
	.specular = {0.5, 0.5, 0.5, 1},
	.roughness = 0.9,
	.metalness = 0.0,
	.opacity = 1,
	.refract_idx = 1.5,

	.diffuse_map         = materialTexture("assets/tex/white.png"),
	.metal_roughness_map = materialTexture("assets/tex/black.png"),
	.normal_map          = materialTexture("assets/tex/lightblue-normal.png"),
	.ambient_occ_map     = materialTexture("assets/tex/white.png"),
};

Texture::ptr default_diffuse, default_metal_roughness;
Texture::ptr default_normmap, default_aomap;

#if 0
static std::map<std::string, material> default_materials = {
	{"(null)", },

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

				   .diffuse_map  = materialTexture("assets/tex/iron-rusted4-Unreal-Engine/iron-rusted4-basecolor.png"),
				   .metal_roughness_map = materialTexture("assets/tex/iron-rusted4-Unreal-Engine/iron-rusted4-metalness.png"),
				   .normal_map   = materialTexture("assets/tex/iron-rusted4-Unreal-Engine/iron-rusted4-normal.png"),
			   }},

	{"Wood",  {
				   .diffuse = {1, 1, 1, 1},
				   .ambient = {1, 1, 1, 1},
				   .specular = {1, 1, 1, 0.3},
				   .roughness = 0.7,
				   .metalness = 0.5,
				   .opacity = 1,
				   .refract_idx = 1.5,

				   .diffuse_map = materialTexture("assets/tex/white.png"),
				   //.diffuse_map  = "assets/tex/rubberduck-tex/199.JPG",
				   //.diffuse_map  = materialTexture("assets/tex/dims/Textures/Boards.JPG"),
				   .metal_roughness_map = materialTexture("assets/tex/white.png"),
				   //.normal_map   = materialTexture("assets/tex/dims/Textures/Textures_N/Boards_N.jpg"),
				   .normal_map = materialTexture("assets/tex/white.png"),
				   //.normal_map   = "assets/tex/rubberduck-tex/199_norm.JPG",
				   .ambient_occ_map = materialTexture("assets/tex/white.png"),
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

				   //.diffuse_map  = materialTexture("assets/tex/Earthmap720x360_grid.jpg"),
				   .diffuse_map = materialTexture("assets/tex/white.png"),
			   }},
};
#endif

renderer::renderer(context& ctx) {
	enable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	Framebuffer().bind();
	DO_ERROR_CHECK();

#ifdef __EMSCRIPTEN__
	screen_x = SCREEN_SIZE_X;
	screen_y = SCREEN_SIZE_Y;
	std::cerr << "also got here" << std::endl;
#else
	SDL_GetWindowSize(ctx.window, &screen_x, &screen_y);
#endif

	framebuffer =
		renderFramebuffer::ptr(new renderFramebuffer(screen_x, screen_y));

	// TODO: skybox should be a setable node object
	//skybox = glman.load_cubemap("assets/tex/cubes/LancellottiChapel/");
	skybox = gen_texture();
	skybox->cubemap("assets/tex/cubes/rocky-skyboxes/Skinnarviksberget/");
	std::cerr << "loaded cubemap" << std::endl;

	default_diffuse         = gen_texture();
	default_metal_roughness = gen_texture();
	default_normmap         = gen_texture();
	default_aomap           = gen_texture();

	default_diffuse->buffer(default_material.diffuse_map, true);
	default_metal_roughness->buffer(default_material.metal_roughness_map);
	default_normmap->buffer(default_material.normal_map);
	default_aomap->buffer(default_material.ambient_occ_map);

#if 0
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
#endif

	loadShaders();

	reflection_atlas = std::unique_ptr<atlas>(new atlas(2048));
	shadow_atlas = std::unique_ptr<atlas>(new atlas(2048, atlas::mode::Depth));

	std::cerr << __func__ << ": Reached end of constructor" << std::endl;
}

void renderer::loadShaders(void) {
	std::cerr << "loading shaders" << std::endl;
	shaders["skybox"] = load_program(
		"shaders/out/skybox.vert",
		"shaders/out/skybox.frag"
	);

	glBindAttribLocation(shaders["skybox"]->obj, 0, "v_position");
	link_program(shaders["skybox"]);

#if GLSL_VERSION < 300
	shaders["main"] = load_program(
		"shaders/out/vertex-shading.vert",
		"shaders/out/vertex-shading.frag"
	);
#else
	shaders["main"] = load_program(
		"shaders/out/pixel-shading.vert",
		//"shaders/out/pixel-shading.frag"
		"shaders/out/pixel-shading-metal-roughness-pbr.frag"
	);
#endif

	shaders["refprobe"] = load_program(
		"shaders/out/ref_probe.vert",
		"shaders/out/ref_probe.frag"
	);

	shaders["refprobe_debug"] = load_program(
		"shaders/out/ref_probe_debug.vert",
		"shaders/out/ref_probe_debug.frag"
	);

	for (auto& name : {"main", "refprobe", "refprobe_debug"}) {
		Program::ptr s = shaders[name];

		glBindAttribLocation(s->obj, 0, "in_Position");
		glBindAttribLocation(s->obj, 1, "v_normal");
		glBindAttribLocation(s->obj, 2, "v_tangent");
		glBindAttribLocation(s->obj, 3, "v_bitangent");
		glBindAttribLocation(s->obj, 4, "texcoord");
		link_program(s);
	}

	shaders["shadow"] = load_program(
		"shaders/out/depth.vert",
		"shaders/out/depth.frag"
	);
	glBindAttribLocation(shaders["shadow"]->obj, 0, "v_position");
	link_program(shaders["shadow"]);

	Vao::ptr orig_vao = get_current_vao();
	bind_vao(get_screenquad_vao());

	shaders["post"] = load_program(
		"shaders/out/postprocess.vert",
		"shaders/out/postprocess.frag"
	);

	glBindAttribLocation(shaders["post"]->obj, 0, "v_position");
	glBindAttribLocation(shaders["post"]->obj, 1, "v_texcoord");
	DO_ERROR_CHECK();
	link_program(shaders["post"]);

	shaders["quadtest"] = load_program(
		"shaders/out/quadtest.vert",
		"shaders/out/quadtest.frag"
	);
	glBindAttribLocation(shaders["quadtest"]->obj, 0, "v_position");
	glBindAttribLocation(shaders["quadtest"]->obj, 1, "v_texcoord");
	DO_ERROR_CHECK();
	link_program(shaders["quadtest"]);

	bind_vao(orig_vao);
	//set_shader(shaders["main"]);
	shaders["main"]->bind();
}

#if 0
void renderer::initFramebuffers(void) {
	// set up the render framebuffer
	rend_fb = gen_framebuffer();
	rend_x = screen_x, rend_y = screen_y;

	//rend.glman.bind_framebuffer(rend_fb);
	rend_fb->bind();
	rend_tex = rend_fb->attach(GL_COLOR_ATTACHMENT0,
	                 gen_texture_color(rend_x, rend_y, GL_RGBA16F));
	rend_depth = rend_fb->attach(GL_DEPTH_STENCIL_ATTACHMENT,
	                 gen_texture_depth_stencil(rend_x, rend_y));

	/*
	// and framebuffer holding the previously drawn frame
	last_frame_fb = gen_framebuffer();
	last_frame_fb->bind();
	last_frame_tex = last_frame_fb->attach(GL_COLOR_ATTACHMENT0,
		                 gen_texture_color(rend_x, rend_y));
						 */
}
#endif

void grendx::set_material(Program::ptr program,
                          compiled_model::ptr obj,
                          std::string mat_name)
{
	if (obj->materials.find(mat_name) == obj->materials.end()) {
		// TODO: maybe show a warning
		set_default_material(program);
		//std::cerr << " ! couldn't find material " << mat_name << std::endl;
		return;
	}

	material& mat = obj->materials[mat_name];

	program->set("anmaterial.diffuse", mat.diffuse);
	program->set("anmaterial.ambient", mat.ambient);
	program->set("anmaterial.specular", mat.specular);
	program->set("anmaterial.roughness", mat.roughness);
	program->set("anmaterial.metalness", mat.metalness);
	program->set("anmaterial.opacity", mat.opacity);

	glActiveTexture(GL_TEXTURE0);
	if (obj->mat_textures.find(mat_name) != obj->mat_textures.end()) {
		obj->mat_textures[mat_name]->bind();
		program->set("diffuse_map", 0);

	} else {
		default_diffuse->bind();
		program->set("diffuse_map", 0);
	}

	glActiveTexture(GL_TEXTURE1);
	if (obj->mat_specular.find(mat_name) != obj->mat_specular.end()) {
		obj->mat_specular[mat_name]->bind();
		program->set("specular_map", 1);

	} else {
		default_metal_roughness->bind();
		program->set("specular_map", 1);
	}

	glActiveTexture(GL_TEXTURE2);
	if (obj->mat_normal.find(mat_name) != obj->mat_normal.end()) {
		obj->mat_normal[mat_name]->bind();
		program->set("normal_map", 2);

	} else {
		default_normmap->bind();
		program->set("normal_map", 2);
	}

	glActiveTexture(GL_TEXTURE3);
	if (obj->mat_ao.find(mat_name) != obj->mat_ao.end()) {
		obj->mat_ao[mat_name]->bind();
		program->set("ambient_occ_map", 3);

	} else {
		default_aomap->bind();
		program->set("ambient_occ_map", 3);
	}

	DO_ERROR_CHECK();
}

void grendx::set_default_material(Program::ptr program) {
	material& mat = default_material;

	program->set("anmaterial.diffuse", mat.diffuse);
	program->set("anmaterial.ambient", mat.ambient);
	program->set("anmaterial.specular", mat.specular);
	program->set("anmaterial.roughness", mat.roughness);
	program->set("anmaterial.metalness", mat.metalness);
	program->set("anmaterial.opacity", mat.opacity);

	glActiveTexture(GL_TEXTURE0);
	default_diffuse->bind();
	program->set("diffuse_map", 0);

	glActiveTexture(GL_TEXTURE1);
	default_metal_roughness->bind();
	// TODO: rename to metal_roughness_map in shaders
	program->set("specular_map", 1);

	glActiveTexture(GL_TEXTURE2);
	default_normmap->bind();
	program->set("normal_map", 2);

	glActiveTexture(GL_TEXTURE3);
	default_aomap->bind();
	program->set("ambient_occ_map", 3);

#if 0
	if (default_materials.find(mat_name) == default_materials.end()) {
		mat_name = fallback_material;
		puts("asdf");
	}

	material& mat = default_materials[mat_name];

	program->set("anmaterial.diffuse", mat.diffuse);
	program->set("anmaterial.ambient", mat.ambient);
	program->set("anmaterial.specular", mat.specular);
	program->set("anmaterial.roughness", mat.roughness);
	program->set("anmaterial.metalness", mat.metalness);
	program->set("anmaterial.opacity", mat.opacity);

	auto loaded_or_fallback = [&] (auto tex) {
		if (tex.loaded()) {
			return mat_name;
		} else {
			return fallback_material;
		}
	};

	glActiveTexture(GL_TEXTURE0);
	diffuse_handles[loaded_or_fallback(mat.diffuse_map)]->bind();
	program->set("diffuse_map", 0);

	glActiveTexture(GL_TEXTURE1);
	specular_handles[loaded_or_fallback(mat.metal_roughness_map)]->bind();
	program->set("specular_map", 1);

	glActiveTexture(GL_TEXTURE2);
	normmap_handles[loaded_or_fallback(mat.normal_map)]->bind();
	program->set("normal_map", 2);

	glActiveTexture(GL_TEXTURE3);
	aomap_handles[loaded_or_fallback(mat.ambient_occ_map)]->bind();
	program->set("ambient_occ_map", 3);
#endif
}

/*
void renderer::set_mvp(glm::mat4 mod, glm::mat4 view, glm::mat4 projection) {
	glm::mat4 v_inv = glm::inverse(view);

	set_m(mod);
	shader->set("v", view);
	shader->set("p", projection);
	shader->set("v_inv", v_inv);
}

void renderer::set_m(glm::mat4 mod) {
	glm::mat3 m_3x3_inv_transp = glm::transpose(glm::inverse(model_to_world(mod)));

	shader->set("m", mod);
	shader->set("m_3x3_inv_transp", m_3x3_inv_transp);
}
*/

glm::mat4 grendx::model_to_world(glm::mat4 model) {
	glm::quat rotation = glm::quat_cast(model);

	return glm::mat4_cast(rotation);
}

/*
// leaving refprobe code commented here so it can be move to the new
// probe code once things are building
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
*/

#if 0
void renderer::draw_mesh(std::string name, const struct draw_attributes *attr) {
	DO_ERROR_CHECK();
	gl_manager::compiled_mesh& foo = glman.cooked_meshes[name];
	set_m(attr->transform);

	// TODO: might be a performance issue, toggleable
	set_reflection_probe(attr);

	if (drawn_counter < 0xff) {
		drawn_entities[drawn_counter] = attr->dclass;
		drawn_counter++;
		glStencilFunc(GL_ALWAYS, drawn_counter, ~0);
	} else {
		glStencilFunc(GL_ALWAYS, 0, ~0);
	}

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
	draw_model(&attr);
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
#endif

void renderer::draw_screenquad(void) {
	// NOTE: assumes that the screenquad vbo has been linked in the
	//       postprocessing shader, and the proper vao set
	glDrawArrays(GL_TRIANGLES, 0, 6);
	DO_ERROR_CHECK();
}

#if 0
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
#endif

#if 0
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
#endif

#if 0
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
#endif
