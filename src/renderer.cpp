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
	.metal_roughness_map = materialTexture("assets/tex/green.png"),
	.normal_map          = materialTexture("assets/tex/lightblue-normal.png"),
	.ambient_occ_map     = materialTexture("assets/tex/white.png"),
};

Texture::ptr default_diffuse, default_metal_roughness;
Texture::ptr default_normmap, default_aomap;

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
	//std::cerr << "loaded cubemap" << std::endl;

	default_diffuse         = gen_texture();
	default_metal_roughness = gen_texture();
	default_normmap         = gen_texture();
	default_aomap           = gen_texture();

	default_diffuse->buffer(default_material.diffuse_map, true);
	default_metal_roughness->buffer(default_material.metal_roughness_map);
	default_normmap->buffer(default_material.normal_map);
	default_aomap->buffer(default_material.ambient_occ_map);

	loadShaders();

	/*
	reflection_atlas = std::unique_ptr<atlas>(new atlas(2048));
	shadow_atlas = std::unique_ptr<atlas>(new atlas(2048, atlas::mode::Depth));
	*/

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

	/*
#if GLSL_VERSION < 300
	shaders["main"] = load_program(
		"shaders/out/vertex-shading.vert",
		"shaders/out/vertex-shading.frag"
	);
#else
	shaders["main"] = load_program(
		"shaders/out/pixel-shading.vert",
		"shaders/out/pixel-shading.frag"
		//"shaders/out/pixel-shading-metal-roughness-pbr.frag"
	);
#endif
*/
	/*
	shaders["main"] = load_program(
		"shaders/out/vertex-shading.vert",
		"shaders/out/vertex-shading.frag"
	);
	*/

	shaders["main"] = load_program(
		"shaders/out/pixel-shading.vert",
		//"shaders/out/pixel-shading.frag"
		"shaders/out/pixel-shading-metal-roughness-pbr.frag"
	);

	shaders["refprobe"] = load_program(
		"shaders/out/ref_probe.vert",
		"shaders/out/ref_probe.frag"
	);

	shaders["refprobe_debug"] = load_program(
		"shaders/out/ref_probe_debug.vert",
		"shaders/out/ref_probe_debug.frag"
	);

	shaders["unshaded"] = load_program(
		"shaders/out/pixel-shading.vert",
		"shaders/out/unshaded.frag"
	);


	for (auto& name : {"main", "refprobe", "refprobe_debug", "unshaded"}) {
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
}

glm::mat4 grendx::model_to_world(glm::mat4 model) {
	glm::quat rotation = glm::quat_cast(model);

	return glm::mat4_cast(rotation);
}

/*
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
*/

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
