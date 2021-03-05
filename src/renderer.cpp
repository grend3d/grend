#include <grend-config.h>

#include <grend/engine.hpp>
#include <grend/gameModel.hpp>
#include <grend/utility.hpp>

#include <vector>
#include <map>
#include <iostream>
#include <algorithm>

using namespace grendx;

material::ptr default_material;
compiledMaterial::ptr default_compiledMat;

renderContext::renderContext(context& ctx) {
	enable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	Framebuffer().bind();
	DO_ERROR_CHECK();

#if GLSL_VERSION >= 140
	lightBuffer = genBuffer(GL_UNIFORM_BUFFER);
	lightBuffer->bind();
	lightBuffer->allocate(sizeof(lights_std140));

	pointTiles = genBuffer(GL_UNIFORM_BUFFER);
	pointTiles->bind();
	pointTiles->allocate(sizeof(light_tiles_std140));

	spotTiles  = genBuffer(GL_UNIFORM_BUFFER);
	spotTiles->bind();
	spotTiles->allocate(sizeof(light_tiles_std140));

	memset(&lightBufferCtx, 0, sizeof(lightBufferCtx));
	memset(&pointTilesCtx,  0, sizeof(pointTilesCtx));
	memset(&spotTilesCtx,   0, sizeof(spotTilesCtx));

	lightBuffer->update(&lightBufferCtx, 0, sizeof(lightBufferCtx));
	pointTiles->update(&pointTilesCtx,   0, sizeof(pointTilesCtx));
	spotTiles->update(&spotTilesCtx,     0, sizeof(spotTilesCtx));
#endif

#ifdef __EMSCRIPTEN__
	screen_x = SCREEN_SIZE_X;
	screen_y = SCREEN_SIZE_Y;
#else
	SDL_GetWindowSize(ctx.window, &screen_x, &screen_y);
#endif

	framebuffer = std::make_shared<renderFramebuffer>(screen_x, screen_y, 4);

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
	/*
	default_material->maps.emissive
		= std::make_shared<materialTexture>(GR_PREFIX "assets/tex/black.png"),
		*/
	default_material->maps.emissive
		= std::make_shared<materialTexture>(GR_PREFIX "assets/tex/white.png"),
	default_material->maps.lightmap
		= std::make_shared<materialTexture>(GR_PREFIX "assets/tex/black.png"),

	default_compiledMat = matcache(default_material);

	loadShaders();
	SDL_Log("Initialized render context");
}

renderFlags grendx::loadShaderToFlags(std::string fragmentPath,
                                      std::string mainVertex,
                                      std::string skinnedVertex,
                                      std::string instancedVertex,
                                      Shader::parameters& options)
{
	renderFlags ret;

	ret.mainShader = loadProgram(mainVertex, fragmentPath, options);
	ret.skinnedShader = loadProgram(skinnedVertex, fragmentPath, options);
	ret.instancedShader = loadProgram(instancedVertex, fragmentPath, options);

	for (Program::ptr prog : {ret.mainShader,
	                          ret.skinnedShader,
	                          ret.instancedShader})
	{
		// TODO: consistent naming, just go with "a_*" since that's what's in
		//       gltf docs, as good a convention as any
		prog->attribute("in_Position", VAO_VERTICES);
		prog->attribute("v_normal",    VAO_NORMALS);
		prog->attribute("v_tangent",   VAO_TANGENTS);
		prog->attribute("v_color",     VAO_COLORS);
		prog->attribute("texcoord",    VAO_TEXCOORDS);
		prog->attribute("a_lightmap",  VAO_LIGHTMAP);
	}

	// skinned shader also has some joint attributes
	ret.skinnedShader->attribute("a_joints",  VAO_JOINTS);
	ret.skinnedShader->attribute("a_weights", VAO_JOINT_WEIGHTS);

	for (Program::ptr prog : {ret.mainShader,
	                          ret.skinnedShader,
	                          ret.instancedShader})
	{
		prog->link();
		DO_ERROR_CHECK();
	}

	return ret;
}

renderFlags grendx::loadLightingShader(std::string fragmentPath,
                                       Shader::parameters& options)
{
	return loadShaderToFlags(fragmentPath,
		GR_PREFIX "shaders/baked/pixel-shading.vert",
		GR_PREFIX "shaders/baked/pixel-shading-skinned.vert",
		GR_PREFIX "shaders/baked/pixel-shading-instanced.vert",
		options);
}

renderFlags grendx::loadProbeShader(std::string fragmentPath,
                                    Shader::parameters& options)
{
	return loadShaderToFlags(fragmentPath,
		// TODO: rename
		GR_PREFIX "shaders/baked/ref_probe.vert",
		GR_PREFIX "shaders/baked/ref_probe-skinned.vert",
		GR_PREFIX "shaders/baked/ref_probe-instanced.vert",
		options);
}

Program::ptr grendx::loadPostShader(std::string fragmentPath,
                                    Shader::parameters& options)
{
	Program::ptr ret = loadProgram(
		GR_PREFIX "shaders/baked/postprocess.vert",
		fragmentPath,
		options
	);

	ret->attribute("v_position", VAO_QUAD_VERTICES);
	ret->attribute("v_texcoord", VAO_QUAD_TEXCOORDS);
	ret->link();

	return ret;
}

void renderContext::loadShaders(void) {
	SDL_Log("Loading shaders");

	lightingShaders["main"] =
		loadLightingShader(
			GR_PREFIX "shaders/baked/pixel-shading-metal-roughness-pbr.frag",
			globalShaderOptions);

	/*
	lightingShaders["main"] =
		loadLightingShader(
			GR_PREFIX "shaders/baked/pixel-shading.frag",
			globalShaderOptions);
			*/

	lightingShaders["unshaded"] = 
		loadLightingShader(
			GR_PREFIX "shaders/baked/unshaded.frag",
			globalShaderOptions);

	lightingShaders["constant-color"] =
		loadLightingShader(
			GR_PREFIX "shaders/baked/constant-color.frag",
			globalShaderOptions);

	probeShaders["refprobe"] =
		loadProbeShader(
			GR_PREFIX "shaders/baked/ref_probe.frag",
			globalShaderOptions);

	probeShaders["shadow"] =
		loadProbeShader(
			GR_PREFIX "shaders/baked/depth.frag",
			globalShaderOptions);

	for (auto& name : {"tonemap", "psaa", "irradiance-convolve",
	                    "specular-convolve", "quadtest"})
	{
		postShaders[name] =
			loadPostShader(
				GR_PREFIX "shaders/baked/" + std::string(name) + ".frag",
				globalShaderOptions);
	}

	internalShaders["refprobe_debug"] = loadProgram(
		GR_PREFIX "shaders/baked/ref_probe_debug.vert",
		GR_PREFIX "shaders/baked/ref_probe_debug.frag",
		globalShaderOptions
	);

	internalShaders["irradprobe_debug"] = loadProgram(
		GR_PREFIX "shaders/baked/ref_probe_debug.vert",
		GR_PREFIX "shaders/baked/irrad_probe_debug.frag",
		globalShaderOptions
	);

	for (auto& name : {"refprobe_debug", "irradprobe_debug"}) {
		Program::ptr s = internalShaders[name];

		s->attribute("in_Position", VAO_VERTICES);
		s->attribute("v_normal",    VAO_NORMALS);
		s->attribute("v_tangent",   VAO_TANGENTS);
		s->attribute("texcoord",    VAO_TEXCOORDS);
		s->link();
	}

	DO_ERROR_CHECK();
}

void renderContext::setArrayMode(enum lightingModes mode) {
	// TODO: buffer allocation and shader recompilation here
	lightingMode = mode;
}

struct renderFlags renderContext::getLightingFlags(std::string name) {
	// TODO: handle name not being in lightingShaders
	return lightingShaders[name];
}

/**
 * Set the reflection probe in a program.
 *
 * NOTE: This isn't used in OpenGL 3.3+ profiles, the reflection probe
 *       is synced in the lighting UBO then.
 */
void renderContext::setReflectionProbe(gameReflectionProbe::ptr probe,
                                       Program::ptr program)
{
	if (!probe) {
		return;
	}

	if (program->cacheObject("reflection_probe", probe.get())) {
		for (unsigned k = 0; k < 5; k++) {
			for (unsigned i = 0; i < 6; i++) {
				std::string sloc =
					"reflection_probe["+std::to_string(k*6 + i)+"]";
				glm::vec3 facevec;

				if (k == 0) {
					facevec = atlases.reflections->tex_vector(probe->faces[k][i]);
				} else {
					facevec = atlases.irradiance->tex_vector(probe->faces[k][i]);
				}

				program->set(sloc, facevec);
				DO_ERROR_CHECK();
			}
		}

		program->set("refboxMin",
		             probe->transform.position + probe->boundingBox.min);
		program->set("refboxMax",
		             probe->transform.position + probe->boundingBox.max);
		program->set("refprobePosition", probe->transform.position);
	}
}

/**
 * Set the irradiance probe in a program.
 */
void renderContext::setIrradianceProbe(gameIrradianceProbe::ptr probe,
                                       Program::ptr program)
{
	if (!probe) {
		return;
	}

	if (program->cacheObject("irradiance_probe", probe.get())) {
		for (unsigned i = 0; i < 6; i++) {
			std::string sloc = "irradiance_probe[" + std::to_string(i) + "]";
			glm::vec3 facevec = atlases.irradiance->tex_vector(probe->faces[i]);
			program->set(sloc, facevec);
			DO_ERROR_CHECK();
		}

		program->set("radboxMin",
		             probe->transform.position + probe->boundingBox.min);
		program->set("radboxMax",
		             probe->transform.position + probe->boundingBox.max);
		program->set("radprobePosition", probe->transform.position);
	}
}

void grendx::set_material(Program::ptr program, compiledMesh::ptr mesh) {
	// XXX: avoid changing everything all at once
	set_material(program, mesh->mat);
}

// TODO: camelCase
void grendx::set_material(Program::ptr program, compiledMaterial::ptr mat) {
	if (!mat) {
		//set_default_material(program);
		// TODO: set default compiled material
		mat = default_compiledMat;
	}

	if (program->cacheObject("current_material", mat.get())) {
		program->set("anmaterial.diffuse",   mat->factors.diffuse);
		program->set("anmaterial.ambient",   mat->factors.ambient);
		program->set("anmaterial.specular",  mat->factors.specular);
		program->set("anmaterial.emissive",  mat->factors.emissive);
		program->set("anmaterial.roughness", mat->factors.roughness);
		program->set("anmaterial.metalness", mat->factors.metalness);
		program->set("anmaterial.opacity",   mat->factors.opacity);

		// with the new-found power of cacheObject, can keep track
		// of which texture object is currently bound
		Texture::ptr diffuse;
		Texture::ptr metalrough;
		Texture::ptr normal;
		Texture::ptr ambientOcclusion;
		Texture::ptr emissive;
		Texture::ptr lightmap;

		diffuse = mat->textures.diffuse
			? mat->textures.diffuse
			: default_compiledMat->textures.diffuse;

		metalrough = mat->textures.metalRoughness
			? mat->textures.metalRoughness
			: default_compiledMat->textures.metalRoughness;

		normal = mat->textures.normal
			? mat->textures.normal
			: default_compiledMat->textures.normal;

		ambientOcclusion = mat->textures.ambientOcclusion
			? mat->textures.ambientOcclusion
			: default_compiledMat->textures.ambientOcclusion;

		emissive = mat->textures.emissive
			? mat->textures.emissive
			: default_compiledMat->textures.emissive;

		lightmap = mat->textures.lightmap
			? mat->textures.lightmap
			: default_compiledMat->textures.lightmap;

		if (program->cacheObject("material_diffuse", diffuse.get())) {
			glActiveTexture(TEX_GL_DIFFUSE);
			diffuse->bind();
		}

		if (program->cacheObject("material_metalrough", metalrough.get())) {
			glActiveTexture(TEX_GL_METALROUGH);
			metalrough->bind();
		}

		if (program->cacheObject("material_normal", normal.get())) {
			glActiveTexture(TEX_GL_NORMAL);
			normal->bind();
		}

		if (program->cacheObject("material_ao", ambientOcclusion.get())) {
			glActiveTexture(TEX_GL_AO);
			ambientOcclusion->bind();
		}

		if (program->cacheObject("material_emissive", emissive.get())) {
			glActiveTexture(TEX_GL_EMISSIVE);
			emissive->bind();
		}

		if (program->cacheObject("material_lightmap", lightmap.get())) {
			glActiveTexture(TEX_GL_LIGHTMAP);
			lightmap->bind();
		}
	}

	// XXX: reusing cacheObject to detect initialization, could have
	//      a isInitialized()
	if (program->cacheObject("material_tex_units", nullptr)) {
		program->set("diffuse_map",     TEXU_DIFFUSE);
		program->set("specular_map",    TEXU_METALROUGH);
		program->set("normal_map",      TEXU_NORMAL);
		program->set("ambient_occ_map", TEXU_AO);
		program->set("emissive_map",    TEXU_EMISSIVE);
		program->set("lightmap",        TEXU_LIGHTMAP);
	}

	DO_ERROR_CHECK();
}

void grendx::set_default_material(Program::ptr program) {
	set_material(program, default_compiledMat);
}

glm::mat4 grendx::model_to_world(glm::mat4 model) {
	glm::quat rotation = glm::quat_cast(model);

	return glm::mat4_cast(rotation);
}

void grendx::packLight(gameLightPoint::ptr light,
                       point_std140 *p,
                       renderContext::ptr rctx,
                       glm::mat4& trans)
{
	glm::vec3 rpos = applyTransform(trans);

	memcpy(p->position, glm::value_ptr(rpos), sizeof(float[3]));
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
                       renderContext::ptr rctx,
                       glm::mat4& trans)
{
	glm::vec3 rpos = applyTransform(trans);
	glm::vec3 rotvec = glm::mat3(trans) * glm::vec3(1, 0, 0);
	glm::vec3 rotup  = glm::mat3(trans) * glm::vec3(0, 1, 0);

	memcpy(p->position,  glm::value_ptr(rpos),           sizeof(float[3]));
	memcpy(p->diffuse,   glm::value_ptr(light->diffuse), sizeof(float[4]));
	memcpy(p->direction, glm::value_ptr(rotvec),         sizeof(float[3]));
	memcpy(p->up,        glm::value_ptr(rotup),          sizeof(float[3]));

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
                       renderContext::ptr rctx,
                       glm::mat4& trans)
{
	glm::vec3 rpos = applyTransform(trans);
	glm::vec3 rotvec =
		glm::mat3_cast(light->transform.rotation) * glm::vec3(1, 0, 0);

	memcpy(p->position, glm::value_ptr(rpos), sizeof(float[3]));
	memcpy(p->diffuse, glm::value_ptr(light->diffuse), sizeof(float[4]));
	memcpy(p->direction, glm::value_ptr(rotvec), sizeof(float[3]));

	p->intensity     = light->intensity;
	p->casts_shadows = light->casts_shadows;

	if (light->casts_shadows) {
		auto vec = rctx->atlases.shadows->tex_vector(light->shadowmap);
		memcpy(p->shadowmap, glm::value_ptr(vec), sizeof(float[3]));
	}
}

void grendx::packRefprobe(gameReflectionProbe::ptr probe,
                          lights_std140 *p,
                          renderContext::ptr rctx,
                          glm::mat4& trans)
{
	if (probe == nullptr) {
		return;
	}

	// TODO: translation
	//glm::vec3 rpos = applyTransform(trans);
	glm::vec3 rpos = probe->transform.position;

	// TODO: configurable number of mipmap levels
	for (unsigned k = 0; k < 5; k++) {
		for (unsigned i = 0; i < 6; i++) {
			glm::vec3 facevec;

			if (k == 0) {
				facevec = rctx->atlases.reflections->tex_vector(probe->faces[k][i]);
			} else {
				facevec = rctx->atlases.irradiance->tex_vector(probe->faces[k][i]);
			}

			memcpy(p->reflection_probe + 4*((6*k) + i),
			       glm::value_ptr(facevec),
			       sizeof(GLfloat[3]));
		}
	}

	glm::vec3 boxmin = rpos + probe->boundingBox.min;
	glm::vec3 boxmax = rpos + probe->boundingBox.max;

	memcpy(p->refboxMin, glm::value_ptr(boxmin), sizeof(GLfloat[3]));
	memcpy(p->refboxMax, glm::value_ptr(boxmax), sizeof(GLfloat[3]));
	memcpy(p->refprobePosition, glm::value_ptr(rpos), sizeof(GLfloat[3]));
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
