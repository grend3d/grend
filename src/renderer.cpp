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
Texture::ptr default_diffuse, default_metal_roughness;
Texture::ptr default_normmap, default_aomap;
Texture::ptr default_emissive;
Texture::ptr default_lightmap;

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

	/*
	for (auto& buf : {pointTiles, spotTiles}) {
		buf->bind();
		buf->allocate(sizeof(light_tiles_std140));
	}
	*/


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
	/*
	default_material->maps.emissive
		= std::make_shared<materialTexture>(GR_PREFIX "assets/tex/black.png"),
		*/
	default_material->maps.emissive
		= std::make_shared<materialTexture>(GR_PREFIX "assets/tex/white.png"),
	default_material->maps.lightmap
		= std::make_shared<materialTexture>(GR_PREFIX "assets/tex/black.png"),

	default_diffuse         = genTexture();
	default_metal_roughness = genTexture();
	default_normmap         = genTexture();
	default_aomap           = genTexture();
	default_emissive        = genTexture();
	default_lightmap        = genTexture();

	default_diffuse->buffer(default_material->maps.diffuse, true);
	default_emissive->buffer(default_material->maps.emissive, true);
	default_lightmap->buffer(default_material->maps.lightmap, true);
	default_metal_roughness->buffer(default_material->maps.metalRoughness);
	default_normmap->buffer(default_material->maps.normal);
	default_aomap->buffer(default_material->maps.ambientOcclusion);

	loadShaders();
	SDL_Log("Initialized render context");
}

renderFlags grendx::loadShaderToFlags(std::string fragmentPath,
                                      std::string mainVertex,
                                      std::string skinnedVertex,
                                      std::string instancedVertex,
                                      shaderOptions& options)
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
                                       shaderOptions& options)
{
	return loadShaderToFlags(fragmentPath,
		GR_PREFIX "shaders/src/pixel-shading.vert",
		GR_PREFIX "shaders/src/pixel-shading-skinned.vert",
		GR_PREFIX "shaders/src/pixel-shading-instanced.vert",
		options);
}

renderFlags grendx::loadProbeShader(std::string fragmentPath,
                                    shaderOptions& options)
{
	return loadShaderToFlags(fragmentPath,
		// TODO: rename
		GR_PREFIX "shaders/src/ref_probe.vert",
		GR_PREFIX "shaders/src/ref_probe-skinned.vert",
		GR_PREFIX "shaders/src/ref_probe-instanced.vert",
		options);
}

Program::ptr grendx::loadPostShader(std::string fragmentPath,
                                    shaderOptions& options)
{
	Program::ptr ret = loadProgram(
		GR_PREFIX "shaders/src/postprocess.vert",
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
			GR_PREFIX "shaders/src/pixel-shading-metal-roughness-pbr.frag",
			globalShaderOptions);

	/*
	lightingShaders["main"] =
		loadLightingShader(
			GR_PREFIX "shaders/src/pixel-shading.frag",
			globalShaderOptions);
			*/

	lightingShaders["unshaded"] = 
		loadLightingShader(
			GR_PREFIX "shaders/src/unshaded.frag",
			globalShaderOptions);

	probeShaders["refprobe"] =
		loadProbeShader(
			GR_PREFIX "shaders/src/ref_probe.frag",
			globalShaderOptions);

	probeShaders["shadow"] =
		loadProbeShader(
			GR_PREFIX "shaders/src/depth.frag",
			globalShaderOptions);

	for (auto& name : {"tonemap", "psaa", "irradiance-convolve",
	                    "specular-convolve", "quadtest"})
	{
		postShaders[name] =
			loadPostShader(
				GR_PREFIX "shaders/src/" + std::string(name) + ".frag",
				globalShaderOptions);
	}

	internalShaders["refprobe_debug"] = loadProgram(
		GR_PREFIX "shaders/src/ref_probe_debug.vert",
		GR_PREFIX "shaders/src/ref_probe_debug.frag",
		globalShaderOptions
	);

	internalShaders["irradprobe_debug"] = loadProgram(
		GR_PREFIX "shaders/src/ref_probe_debug.vert",
		GR_PREFIX "shaders/src/irrad_probe_debug.frag",
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
	glm::vec3 rotvec =
		glm::mat3_cast(light->transform.rotation) * glm::vec3(1, 0, 0);

	memcpy(p->position, glm::value_ptr(rpos), sizeof(float[3]));
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
