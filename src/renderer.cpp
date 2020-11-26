#include <grend/engine.hpp>
#include <grend/gameModel.hpp>
#include <grend/utility.hpp>

#include <vector>
#include <map>
#include <iostream>
#include <algorithm>

using namespace grendx;

material::ptr default_material;
Texture::ptr default_diffuse, default_metal_roughness;
Texture::ptr default_normmap, default_aomap;
Texture::ptr default_emissive;

renderContext::renderContext(context& ctx) {
	enable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	Framebuffer().bind();
	DO_ERROR_CHECK();

#if GLSL_VERSION >= 140
	lightBuffer = genBuffer(GL_UNIFORM_BUFFER);
	lightBuffer->bind();
	lightBuffer->allocate(sizeof(lights_std140));
#endif

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
	//
	default_material = std::make_shared<material>();
	default_material->factors.diffuse = {1, 1, 1, 1},
	default_material->factors.ambient = {1, 1, 1, 1},
	default_material->factors.specular = {1, 1, 1, 1},
	default_material->factors.emissive = {1, 1, 1, 1},
	default_material->factors.roughness = 1.f,
	default_material->factors.metalness = 0.f,
	default_material->factors.opacity = 1,
	default_material->factors.refract_idx = 1.5,

	default_material->maps.diffuse
		= std::make_shared<materialTexture>(GR_PREFIX "assets/tex/white.png"),
	default_material->maps.metalRoughness
		= std::make_shared<materialTexture>(GR_PREFIX "assets/tex/white.png"),
	default_material->maps.normal
		= std::make_shared<materialTexture>(GR_PREFIX "assets/tex/lightblue-normal.png"),
	default_material->maps.ambientOcclusion
		= std::make_shared<materialTexture>(GR_PREFIX "assets/tex/white.png"),
	default_material->maps.emissive
		= std::make_shared<materialTexture>(GR_PREFIX "assets/tex/black.png"),

	default_diffuse         = genTexture();
	default_metal_roughness = genTexture();
	default_normmap         = genTexture();
	default_aomap           = genTexture();
	default_emissive        = genTexture();

	default_diffuse->buffer(default_material->maps.diffuse, true);
	default_emissive->buffer(default_material->maps.emissive, true);
	default_metal_roughness->buffer(default_material->maps.metalRoughness);
	default_normmap->buffer(default_material->maps.normal);
	default_aomap->buffer(default_material->maps.ambientOcclusion);

	loadShaders();
	std::cerr << __func__ << ": Reached end of constructor" << std::endl;
}

void renderContext::loadShaders(void) {
	std::cerr << "loading shaders" << std::endl;
	/*
	shaders["main"] = loadProgram(
		GR_PREFIX "shaders/out/vertex-shading.vert",
		GR_PREFIX "shaders/out/vertex-shading.frag"
	);
	*/

	shaders["main"] = loadProgram(
		GR_PREFIX "shaders/out/pixel-shading.vert",
		//GR_PREFIX "shaders/out/pixel-shading.frag"
		GR_PREFIX "shaders/out/pixel-shading-metal-roughness-pbr.frag"
	);

	shaders["main-skinned"] = loadProgram(
		GR_PREFIX "shaders/out/pixel-shading-skinned.vert",
		//GR_PREFIX "shaders/out/pixel-shading.frag"
		GR_PREFIX "shaders/out/pixel-shading-metal-roughness-pbr.frag"
	);

	shaders["refprobe"] = loadProgram(
		GR_PREFIX "shaders/out/ref_probe.vert",
		GR_PREFIX "shaders/out/ref_probe.frag"
	);

	// TODO: skinned vertex shader
	shaders["refprobe-skinned"] = loadProgram(
		GR_PREFIX "shaders/out/ref_probe.vert",
		GR_PREFIX "shaders/out/ref_probe.frag"
	);

	shaders["refprobe_debug"] = loadProgram(
		GR_PREFIX "shaders/out/ref_probe_debug.vert",
		GR_PREFIX "shaders/out/ref_probe_debug.frag"
	);

	shaders["irradprobe_debug"] = loadProgram(
		GR_PREFIX "shaders/out/ref_probe_debug.vert",
		GR_PREFIX "shaders/out/irrad_probe_debug.frag"
	);

	shaders["unshaded"] = loadProgram(
		GR_PREFIX "shaders/out/pixel-shading.vert",
		GR_PREFIX "shaders/out/unshaded.frag"
	);

	for (auto& name : {"main", "refprobe", "refprobe_debug",
	                   "irradprobe_debug", "unshaded"})
	{
		Program::ptr s = shaders[name];

		s->attribute("in_Position", VAO_VERTICES);
		s->attribute("v_normal",    VAO_NORMALS);
		s->attribute("v_tangent",   VAO_TANGENTS);
		s->attribute("v_bitangent", VAO_BITANGENTS);
		s->attribute("texcoord",    VAO_TEXCOORDS);
		s->link();
	}

	shaders["main-skinned"]->attribute("in_Position", VAO_VERTICES);
	shaders["main-skinned"]->attribute("v_normal",    VAO_NORMALS);
	shaders["main-skinned"]->attribute("v_tangent",   VAO_TANGENTS);
	shaders["main-skinned"]->attribute("v_bitangent", VAO_BITANGENTS);
	shaders["main-skinned"]->attribute("texcoord",    VAO_TEXCOORDS);
	shaders["main-skinned"]->attribute("a_joints",    VAO_JOINTS);
	shaders["main-skinned"]->attribute("a_weights",   VAO_JOINT_WEIGHTS);
	shaders["main-skinned"]->link();

	shaders["refprobe-skinned"]->attribute("in_Position", VAO_VERTICES);
	shaders["refprobe-skinned"]->attribute("v_normal",    VAO_NORMALS);
	shaders["refprobe-skinned"]->attribute("v_tangent",   VAO_TANGENTS);
	shaders["refprobe-skinned"]->attribute("v_bitangent", VAO_BITANGENTS);
	shaders["refprobe-skinned"]->attribute("texcoord",    VAO_TEXCOORDS);
	shaders["refprobe-skinned"]->attribute("a_joints",    VAO_JOINTS);
	shaders["refprobe-skinned"]->attribute("a_weights",   VAO_JOINT_WEIGHTS);
	shaders["refprobe-skinned"]->link();

	shaders["shadow"] = loadProgram(
		GR_PREFIX "shaders/out/depth.vert",
		GR_PREFIX "shaders/out/depth.frag"
	);
	shaders["shadow"]->attribute("v_position", VAO_VERTICES);
	shaders["shadow"]->link();

	// TODO: skinned vertex shader
	shaders["shadow-skinned"] = loadProgram(
		GR_PREFIX "shaders/out/depth.vert",
		GR_PREFIX "shaders/out/depth.frag"
	);
	shaders["shadow-skinned"]->attribute("v_position", VAO_VERTICES);
	shaders["shadow-skinned"]->link();

	shaders["tonemap"] = loadProgram(
		GR_PREFIX "shaders/out/postprocess.vert",
		GR_PREFIX "shaders/out/tonemap.frag"
	);

	// NOTE: post
	shaders["tonemap"]->attribute("v_position", VAO_QUAD_VERTICES);
	shaders["tonemap"]->attribute("v_texcoord", VAO_QUAD_TEXCOORDS);
	shaders["tonemap"]->link();

	shaders["psaa"] = loadProgram(
		GR_PREFIX "shaders/out/postprocess.vert",
		GR_PREFIX "shaders/out/psaa.frag"
	);

	// NOTE: post
	shaders["psaa"]->attribute("v_position", VAO_QUAD_VERTICES);
	shaders["psaa"]->attribute("v_texcoord", VAO_QUAD_TEXCOORDS);
	shaders["psaa"]->link();

	shaders["irradiance-convolve"] = loadProgram(
		GR_PREFIX "shaders/out/postprocess.vert",
		GR_PREFIX "shaders/out/irradiance-convolve.frag"
	);

	// NOTE: post
	shaders["irradiance-convolve"]->attribute("v_position", VAO_QUAD_VERTICES);
	shaders["irradiance-convolve"]->attribute("v_texcoord", VAO_QUAD_TEXCOORDS);
	shaders["irradiance-convolve"]->link();

	shaders["specular-convolve"] = loadProgram(
		GR_PREFIX "shaders/out/postprocess.vert",
		GR_PREFIX "shaders/out/specular-convolve.frag"
	);
	shaders["specular-convolve"]->attribute("v_position", VAO_QUAD_VERTICES);
	shaders["specular-convolve"]->attribute("v_texcoord", VAO_QUAD_TEXCOORDS);
	shaders["specular-convolve"]->link();

	shaders["quadtest"] = loadProgram(
		GR_PREFIX "shaders/out/quadtest.vert",
		GR_PREFIX "shaders/out/quadtest.frag"
	);
	shaders["quadtest"]->attribute("v_position", VAO_QUAD_VERTICES);
	shaders["quadtest"]->attribute("v_texcoord", VAO_QUAD_TEXCOORDS);
	shaders["quadtest"]->link();

	shaders["main"]->bind();
}

void renderContext::setFlags(const renderFlags& newflags) {
	flags = newflags;
}

struct renderFlags renderContext::getDefaultFlags(std::string name) {
	return (struct renderFlags) {
		// TODO: some sort of shader class/interface/whatever system
		//       so that fragment shaders can be automatically paired with
		//       compatible vertex shaders
		//       maybe just do a (c++) class-based thing?
		.mainShader    = shaders[name],
		.skinnedShader = shaders[name+"-skinned"],
	};
}

const renderFlags& renderContext::getFlags(void) {
	return flags;
}

void grendx::set_material(Program::ptr program, compiledMesh::ptr mesh) {
	material::materialFactors& mat = mesh->factors;

	program->set("anmaterial.diffuse",   mat.diffuse);
	program->set("anmaterial.ambient",   mat.ambient);
	program->set("anmaterial.specular",  mat.specular);
	program->set("anmaterial.emissive",  mat.emissive);
	program->set("anmaterial.roughness", mat.roughness);
	program->set("anmaterial.metalness", mat.metalness);
	program->set("anmaterial.opacity",   mat.opacity);

	glActiveTexture(GL_TEXTURE0);
	if (mesh->textures.diffuse) {
		mesh->textures.diffuse->bind();
		program->set("diffuse_map", 0);

	} else {
		default_diffuse->bind();
		program->set("diffuse_map", 0);
	}

	glActiveTexture(GL_TEXTURE1);
	if (mesh->textures.metalRoughness) {
		mesh->textures.metalRoughness->bind();
		program->set("specular_map", 1);

	} else {
		default_metal_roughness->bind();
		program->set("specular_map", 1);
	}

	glActiveTexture(GL_TEXTURE2);
	if (mesh->textures.normal) {
		mesh->textures.normal->bind();
		program->set("normal_map", 2);

	} else {
		default_normmap->bind();
		program->set("normal_map", 2);
	}

	glActiveTexture(GL_TEXTURE3);
	if (mesh->textures.ambientOcclusion) {
		mesh->textures.ambientOcclusion->bind();
		program->set("ambient_occ_map", 3);

	} else {
		default_aomap->bind();
		program->set("ambient_occ_map", 3);
	}

	glActiveTexture(GL_TEXTURE4);
	if (mesh->textures.emissive) {
		//obj->matEmissive[mat_name]->bind();
		mesh->textures.emissive->bind();
		program->set("emissive_map", 4);

	} else {
		default_emissive->bind();
		program->set("emissive_map", 4);
	}

	DO_ERROR_CHECK();
}

void grendx::set_default_material(Program::ptr program) {
	material::ptr mat = default_material;

	program->set("anmaterial.diffuse",   mat->factors.diffuse);
	program->set("anmaterial.ambient",   mat->factors.ambient);
	program->set("anmaterial.specular",  mat->factors.specular);
	program->set("anmaterial.emissive",  mat->factors.emissive);
	program->set("anmaterial.roughness", mat->factors.roughness);
	program->set("anmaterial.metalness", mat->factors.metalness);
	program->set("anmaterial.opacity",   mat->factors.opacity);

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

	glActiveTexture(GL_TEXTURE4);
	default_emissive->bind();
	program->set("emissive_map", 4);
}

glm::mat4 grendx::model_to_world(glm::mat4 model) {
	glm::quat rotation = glm::quat_cast(model);

	return glm::mat4_cast(rotation);
}

void grendx::packLight(gameLightPoint::ptr light,
                       point_std140 *p,
                       renderContext::ptr rctx)
{
	memcpy(p->position, glm::value_ptr(light->transform.position), sizeof(float[3]));
	memcpy(p->diffuse, glm::value_ptr(light->diffuse), sizeof(float[4]));
	p->intensity = light->intensity;
	p->radius    = light->radius;
	p->casts_shadows = light->casts_shadows;

	if (light->casts_shadows) {
		for (unsigned i = 0; i < 6; i++) {
			auto vec = rctx->atlases.shadows->tex_vector(light->shadowmap[i]);
			memcpy(p->shadowmap + 4*i, glm::value_ptr(vec), sizeof(float[3]));
		}
	}
}

void grendx::packLight(gameLightSpot::ptr light,
                       spot_std140 *p,
                       renderContext::ptr rctx)
{
	glm::vec3 rotvec =
		glm::mat3_cast(light->transform.rotation) * glm::vec3(1, 0, 0);

	memcpy(p->position, glm::value_ptr(light->transform.position), sizeof(float[3]));
	memcpy(p->diffuse, glm::value_ptr(light->diffuse), sizeof(float[4]));
	memcpy(p->direction, glm::value_ptr(rotvec), sizeof(float[3]));

	p->intensity     = light->intensity;
	p->radius        = light->radius;
	p->angle         = light->angle;
	p->casts_shadows = light->casts_shadows;

	if (light->casts_shadows) {
		auto vec = rctx->atlases.shadows->tex_vector(light->shadowmap);
		memcpy(p->shadowmap, glm::value_ptr(vec), sizeof(float[3]));
	}
}

void grendx::packLight(gameLightDirectional::ptr light,
                       directional_std140 *p,
                       renderContext::ptr rctx)
{
	glm::vec3 rotvec =
		glm::mat3_cast(light->transform.rotation) * glm::vec3(1, 0, 0);

	memcpy(p->position, glm::value_ptr(light->transform.position), sizeof(float[3]));
	memcpy(p->diffuse, glm::value_ptr(light->diffuse), sizeof(float[4]));
	memcpy(p->direction, glm::value_ptr(rotvec), sizeof(float[3]));

	p->intensity     = light->intensity;
	p->casts_shadows = light->casts_shadows;

	if (light->casts_shadows) {
		auto vec = rctx->atlases.shadows->tex_vector(light->shadowmap);
		memcpy(p->shadowmap, glm::value_ptr(vec), sizeof(float[3]));
	}
}

void grendx::invalidateLightMaps(gameObject::ptr tree) {
	if (tree->type == gameObject::objType::Light) {
		gameLight::ptr light = std::dynamic_pointer_cast<gameLight>(tree);
		light->have_map = false;

	// TODO: reflection probes and irradiance probes are basically the
	//       same structure, should have a base class they derive from
	} else if (tree->type == gameObject::objType::ReflectionProbe) {
		gameReflectionProbe::ptr probe
			= std::dynamic_pointer_cast<gameReflectionProbe>(tree);
		probe->have_map = false;

	} else if (tree->type == gameObject::objType::IrradianceProbe) {
		gameIrradianceProbe::ptr probe
			= std::dynamic_pointer_cast<gameIrradianceProbe>(tree);
		probe->have_map = false;
	}

	for (auto& [name, node] : tree->nodes) {
		invalidateLightMaps(node);
	}
}
