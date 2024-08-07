#include <grend-config.h>

#include <grend/engine.hpp>
#include <grend/sceneModel.hpp>
#include <grend/utility.hpp>
#include <grend/logger.hpp>

#include <vector>
#include <map>
#include <iostream>
#include <algorithm>
#include <array>

using namespace grendx;

material default_material;
compiledMaterial::ptr default_compiledMat;

void renderContext::applySettings(const renderSettings& newsettings) {
	settings = newsettings;

	atlases = renderAtlases(settings.reflectionAtlasSize,
	                        settings.shadowAtlasSize,
	                        settings.lightProbeAtlasSize);

	unsigned adjX = settings.targetResX * settings.scaleX;
	unsigned adjY = settings.targetResY * settings.scaleY;
	framebuffer = std::make_shared<renderFramebuffer>(adjX, adjY, settings.msaaLevel);
}

renderContext::renderContext(SDLContext& ctx, const renderSettings& _settings) {
	applySettings(_settings);

	Framebuffer().bind();
	DO_ERROR_CHECK();

	defaultFramebuffer = std::make_shared<renderFramebuffer>();

	// TODO: something like this would be ideal
	//globalShaderOptions["glsl_version"] = GLSL_VERSION;
	//globalShaderOptions["max_lights"]   = MAX_LIGHTS;

#if GLSL_VERSION >= 140
	LogFmt("GOT HERE!!!!!!!!!!!!!!!!!!");
	LogFmt("renderContext(): sizeof(lights_std140): {}", sizeof(lights_std140));
	LogFmt("renderContext(): sizeof(light_tiles_std140): {}", sizeof(light_tiles_std140));
	LogFmt("renderContext(): sizeof(point_light_buffer_std140): {}", sizeof(point_light_buffer_std140));
	LogFmt("renderContext(): sizeof(spot_light_buffer_std140): {}", sizeof(spot_light_buffer_std140));
	LogFmt("renderContext(): sizeof(directional_light_buffer_std140): {}", sizeof(directional_light_buffer_std140));

	lightBuffer = genBuffer(GL_UNIFORM_BUFFER);
	lightBuffer->bind();
	lightBuffer->allocate(sizeof(lights_std140));

	pointTiles = genBuffer(GL_UNIFORM_BUFFER);
	pointTiles->bind();
	pointTiles->allocate(sizeof(light_tiles_std140));

	spotTiles  = genBuffer(GL_UNIFORM_BUFFER);
	spotTiles->bind();
	spotTiles->allocate(sizeof(light_tiles_std140));

	pointBuffer = genBuffer(GL_UNIFORM_BUFFER);
	pointBuffer->bind();
	pointBuffer->allocate(sizeof(point_light_buffer_std140));

	spotBuffer = genBuffer(GL_UNIFORM_BUFFER);
	spotBuffer->bind();
	spotBuffer->allocate(sizeof(spot_light_buffer_std140));

	directionalBuffer = genBuffer(GL_UNIFORM_BUFFER);
	directionalBuffer->bind();
	directionalBuffer->allocate(sizeof(directional_light_buffer_std140));


	memset(&lightBufferCtx,       0, sizeof(lightBufferCtx));
	memset(&pointTilesCtx,        0, sizeof(pointTilesCtx));
	memset(&spotTilesCtx,         0, sizeof(spotTilesCtx));
	memset(&pointLightsCtx,       0, sizeof(pointLightsCtx));
	memset(&spotLightsCtx,        0, sizeof(spotLightsCtx));
	memset(&directionalLightsCtx, 0, sizeof(directionalLightsCtx));

	lightBuffer->update(&lightBufferCtx, 0, sizeof(lightBufferCtx));
	pointTiles->update(&pointTilesCtx,   0, sizeof(pointTilesCtx));
	spotTiles->update(&spotTilesCtx,     0, sizeof(spotTilesCtx));

	pointBuffer      ->update(&pointLightsCtx,       0, sizeof(pointLightsCtx));
	spotBuffer       ->update(&spotLightsCtx,        0, sizeof(spotLightsCtx));
	directionalBuffer->update(&directionalLightsCtx, 0, sizeof(directionalLightsCtx));
#endif

	SDL_GetWindowSize(ctx.window, &screenX, &screenY);
	/*
#ifdef __EMSCRIPTEN__
	screen_x = 1280;
	screen_y = 720;
#else
	SDL_GetWindowSize(ctx.window, &screen_x, &screen_y);
#endif
*/

	// TODO: skybox should be a setable node object
	//skybox = glman.load_cubemap("assets/tex/cubes/LancellottiChapel/");
	//
	default_material.factors.diffuse = {1, 1, 1, 1},
	default_material.factors.ambient = {1, 1, 1, 1},
	default_material.factors.specular = {1, 1, 1, 1},
	default_material.factors.emissive = {1, 1, 1, 1},
	default_material.factors.roughness = 1.f,
	default_material.factors.metalness = 0.f,
	default_material.factors.opacity = 1,
	default_material.factors.refract_idx = 1.5,

	default_material.maps.diffuse
		= std::make_shared<textureData>(GR_PREFIX "assets/tex/white.png"),
	default_material.maps.metalRoughness
		= std::make_shared<textureData>(GR_PREFIX "assets/tex/white.png"),
	default_material.maps.normal
		= std::make_shared<textureData>(GR_PREFIX "assets/tex/lightblue-normal.png"),
	default_material.maps.ambientOcclusion
		= std::make_shared<textureData>(GR_PREFIX "assets/tex/white.png"),
	/*
	default_material.maps.emissive
		= std::make_shared<textureData>(GR_PREFIX "assets/tex/black.png"),
		*/
	default_material.maps.emissive
		= std::make_shared<textureData>(GR_PREFIX "assets/tex/white.png"),
	default_material.maps.lightmap
		= std::make_shared<textureData>(GR_PREFIX "assets/tex/black.png"),

	default_compiledMat = matcache(&default_material);

	loadShaders();
	LogInfo("Initialized render context");
}

renderFlags grendx::loadShaderToFlags(std::string fragPath,
                                      std::string mainVertex,
                                      std::string skinnedVertex,
                                      std::string instancedVertex,
                                      std::string billboardVertex,
                                      const Shader::parameters& opts)
{
	renderFlags ret;

	LogFmt("Loading shaders for fragment shader {}", fragPath);

	using R = renderFlags;

	static const std::array<std::string, R::MaxVariants> modestrs = {
		"opaque",
		"blend",
		"masked",
	};

	for (unsigned i = 0; i < R::MaxVariants; i++) {
		const std::string str = modestrs[i];
		Shader::parameters usropts = {
			{"BLEND_MODE_OPAQUE",          (GLint)(i == R::Opaque)},
			{"BLEND_MODE_DITHERED_BLEND",  (GLint)(i == R::DitheredBlend)},
			{"BLEND_MODE_MASKED",          (GLint)(i == R::Masked)},
		};
		//{"BLEND_MODE_OPAQUE" + modestrs[i], 1}};
		auto usr = mergeOpts({opts, usropts});
		auto& var = ret.variants[i];

		var.shaders[R::Main]      = loadProgram(mainVertex,      fragPath, usr);
		var.shaders[R::Skinned]   = loadProgram(skinnedVertex,   fragPath, usr);
		var.shaders[R::Instanced] = loadProgram(instancedVertex, fragPath, usr);
		var.shaders[R::Billboard] = loadProgram(billboardVertex, fragPath, usr);
	}

	for (auto& var : ret.variants) {
		for (unsigned i = 0; i < R::MaxShaders; i++) {
			// TODO: consistent naming, just go with "a_*" since that's what's in
			//       gltf docs, as good a convention as any
			var.shaders[i]->attribute("in_Position", VAO_VERTICES);
			var.shaders[i]->attribute("v_normal",    VAO_NORMALS);
			var.shaders[i]->attribute("v_tangent",   VAO_TANGENTS);
			var.shaders[i]->attribute("v_color",     VAO_COLORS);
			var.shaders[i]->attribute("texcoord",    VAO_TEXCOORDS);
			var.shaders[i]->attribute("a_lightmap",  VAO_LIGHTMAP);
		}

		// skinned shader also has some joint attributes
		var.shaders[R::Skinned]->attribute("a_joints",  VAO_JOINTS);
		var.shaders[R::Skinned]->attribute("a_weights", VAO_JOINT_WEIGHTS);
	}

	for (auto& var : ret.variants) {
		for (unsigned i = 0; i < R::MaxShaders; i++) {
			var.shaders[i]->link();
			DO_ERROR_CHECK();
		}
	}

	return ret;
}

renderFlags grendx::loadLightingShader(std::string fragmentPath,
                                       const Shader::parameters& options)
{
	return loadShaderToFlags(fragmentPath,
		GR_PREFIX "shaders/baked/pixel-shading.vert",
		GR_PREFIX "shaders/baked/pixel-shading-skinned.vert",
		GR_PREFIX "shaders/baked/pixel-shading-instanced.vert",
		GR_PREFIX "shaders/baked/pixel-shading-billboard.vert",
		options);
}

renderFlags grendx::loadProbeShader(std::string fragmentPath,
                                    const Shader::parameters& options)
{
	return loadShaderToFlags(fragmentPath,
		// TODO: rename
		GR_PREFIX "shaders/baked/ref_probe.vert",
		GR_PREFIX "shaders/baked/ref_probe-skinned.vert",
		GR_PREFIX "shaders/baked/ref_probe-instanced.vert",
		GR_PREFIX "shaders/baked/ref_probe-billboard.vert",
		options);
}

Program::ptr grendx::loadPostShader(std::string fragmentPath,
                                    const Shader::parameters& options)
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
	LogInfo("Loading shaders");

	lightingShaders["pixel-metalroughness"] =
		loadLightingShader(
			GR_PREFIX "shaders/baked/pixel-shading-metal-roughness-pbr.frag",
			globalShaderOptions);

	lightingShaders["pixel-matcap"] =
		loadLightingShader(
			GR_PREFIX "shaders/baked/pixel-shading-matcap.frag",
			globalShaderOptions);

	lightingShaders["pixel-normal"] =
		loadLightingShader(
			GR_PREFIX "shaders/baked/normals.frag",
			globalShaderOptions);

	lightingShaders["vertex-metalroughness"] =
		loadShaderToFlags(GR_PREFIX "shaders/baked/vertex-shading.frag",
			GR_PREFIX "shaders/baked/vertex-shading.vert",
			GR_PREFIX "shaders/baked/vertex-shading-skinned.vert",
			GR_PREFIX "shaders/baked/vertex-shading-instanced.vert",
			GR_PREFIX "shaders/baked/vertex-shading-billboard.vert",
			globalShaderOptions);

	lightingShaders["main"] = lightingShaders["pixel-metalroughness"];

	lightingShaders["pixel-blinn-phong"] =
		loadLightingShader(
			GR_PREFIX "shaders/baked/pixel-shading.frag",
			globalShaderOptions);

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
	                    "specular-convolve", "quadtest", "fog-depth"})
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
	if (name.empty()) {
		return lightingShaders[defaultLighting];

	} else {
		// TODO: handle name not being in lightingShaders
		return lightingShaders[name];
	}
}

/**
 * Set the reflection probe in a program.
 *
 * NOTE: This isn't used in OpenGL 3.3+ profiles, the reflection probe
 *       is synced in the lighting UBO then.
 */
void renderContext::setReflectionProbe(sceneReflectionProbe::ptr probe,
                                       Program::ptr program)
{
	if (!probe) {
		return;
	}

	if (program->cacheObject("reflection_probe", probe.getPtr())) {
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

		const TRS& transform = probe->transform.getTRS();

		program->set("refboxMin", transform.position + probe->boundingBox.min);
		program->set("refboxMax", transform.position + probe->boundingBox.max);
		program->set("refprobePosition", transform.position);
	}
}

/**
 * Set the irradiance probe in a program.
 */
void renderContext::setIrradianceProbe(sceneIrradianceProbe::ptr probe,
                                       Program::ptr program)
{
	if (!probe) {
		return;
	}

	if (program->cacheObject("irradiance_probe", probe.getPtr())) {
		for (unsigned i = 0; i < 6; i++) {
			std::string sloc = "irradiance_probe[" + std::to_string(i) + "]";
			glm::vec3 facevec = atlases.irradiance->tex_vector(probe->faces[i]);
			program->set(sloc, facevec);
			DO_ERROR_CHECK();
		}

		const TRS& transform = probe->transform.getTRS();
		program->set("radboxMin", transform.position + probe->boundingBox.min);
		program->set("radboxMax", transform.position + probe->boundingBox.max);
		program->set("radprobePosition", transform.position);
	}
}

void renderContext::setDefaultLightModel(std::string name) {
	auto it = lightingShaders.find(name);

	if (it != lightingShaders.end()) {
		defaultLighting = name;
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
		// TODO: UBOs for materialis
		program->set("anmaterial.diffuse",     mat->factors.diffuse);
		program->set("anmaterial.ambient",     mat->factors.ambient);
		program->set("anmaterial.specular",    mat->factors.specular);
		program->set("anmaterial.emissive",    mat->factors.emissive);
		program->set("anmaterial.roughness",   mat->factors.roughness);
		program->set("anmaterial.metalness",   mat->factors.metalness);
		program->set("anmaterial.opacity",     mat->factors.opacity);
		program->set("anmaterial.alphaCutoff", mat->factors.alphaCutoff);

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
			// TODO: could have setter functions for simple types
			//       (or just do value caching in set())
			bool is_vec = diffuse->type == textureData::imageType::VecTex;
			if (program->cacheObject("diffuse-is-vec", is_vec)) {
				program->set("diffuse_vec", is_vec);
			}

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
			bool is_vec = emissive->type == textureData::imageType::VecTex;
			if (program->cacheObject("emissive-is-vec", is_vec)) {
				program->set("emissive_vec", is_vec);
			}

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

void grendx::packLight(sceneLightPoint::ptr light,
                       point_std140 *p,
                       renderContext *rctx,
                       glm::mat4& trans)
{
	glm::vec3 rpos = extractTranslation(trans);

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

void grendx::packLight(sceneLightSpot::ptr light,
                       spot_std140 *p,
                       renderContext *rctx,
                       glm::mat4& trans)
{
	glm::vec3 rpos = extractTranslation(trans);
	glm::vec3 rotvec = glm::mat3(trans) * glm::vec3(0, 0, -1);

	memcpy(p->position,  glm::value_ptr(rpos),           sizeof(float[3]));
	memcpy(p->diffuse,   glm::value_ptr(light->diffuse), sizeof(float[4]));
	memcpy(p->direction, glm::value_ptr(rotvec),         sizeof(float[3]));

	p->intensity     = light->intensity;
	p->radius        = light->radius;
	p->angle         = light->angle;
	p->casts_shadows = light->casts_shadows;

	if (light->casts_shadows) {
		auto vec = rctx->atlases.shadows->tex_vector(light->shadowmap);
		memcpy(p->shadowmap,  glm::value_ptr(vec), sizeof(float[3]));
		memcpy(p->shadowproj, glm::value_ptr(light->shadowproj), sizeof(float[16]));
	}
}

void grendx::packLight(sceneLightDirectional::ptr light,
                       directional_std140 *p,
                       renderContext *rctx,
                       glm::mat4& trans)
{
	glm::vec3 rpos = extractTranslation(trans);
	glm::vec3 rotvec =
		glm::mat3_cast(light->transform.getTRS().rotation) * glm::vec3(1, 0, 0);

	memcpy(p->position, glm::value_ptr(rpos), sizeof(float[3]));
	memcpy(p->diffuse, glm::value_ptr(light->diffuse), sizeof(float[4]));
	memcpy(p->direction, glm::value_ptr(rotvec), sizeof(float[3]));

	p->intensity     = light->intensity;
	p->casts_shadows = light->casts_shadows;

	if (light->casts_shadows) {
		auto vec = rctx->atlases.shadows->tex_vector(light->shadowmap);
		memcpy(p->shadowmap, glm::value_ptr(vec), sizeof(float[3]));
		memcpy(p->shadowproj, glm::value_ptr(light->shadowproj), sizeof(float[16]));
	}
}

void grendx::packRefprobe(sceneReflectionProbe::ptr probe,
                          lights_std140 *p,
                          renderContext *rctx,
                          glm::mat4& trans)
{
	if (probe == nullptr) {
		return;
	}

	// TODO: translation
	//glm::vec3 rpos = applyTransform(trans);
	glm::vec3 rpos = probe->transform.getTRS().position;

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

void grendx::invalidateLightMaps(sceneNode::ptr tree) {
	if (tree->type == sceneNode::objType::Light) {
		sceneLight::ptr light = ref_cast<sceneLight>(tree);
		light->have_map = false;

	// TODO: reflection probes and irradiance probes are basically the
	//       same structure, should have a base class they derive from
	} else if (tree->type == sceneNode::objType::ReflectionProbe) {
		sceneReflectionProbe::ptr probe = ref_cast<sceneReflectionProbe>(tree);
		probe->have_map = false;

	} else if (tree->type == sceneNode::objType::IrradianceProbe) {
		sceneIrradianceProbe::ptr probe = ref_cast<sceneIrradianceProbe>(tree);
		probe->have_map = false;
	}

	for (auto node : tree->nodes()) {
		invalidateLightMaps(node->getRef());
	}
}
