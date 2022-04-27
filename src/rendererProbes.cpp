#include <grend/engine.hpp>
#include <grend/sceneModel.hpp>
#include <grend/utility.hpp>
#include <grend/textureAtlas.hpp>
#include <grend/timers.hpp>

using namespace grendx;

static const glm::vec3 cube_dirs[] = {
	// negative X, Y, Z
	{-1,  0,  0},
	{ 0, -1,  0},
	{ 0,  0, -1},

	// positive X, Y, Z
	{ 1,  0,  0},
	{ 0,  1,  0},
	{ 0,  0,  1},
};

static const glm::vec3 cube_up[] = {
	// negative X, Y, Z
	{ 0,  1,  0},
	{ 0,  0,  1},
	{ 0,  1,  0},

	// positive X, Y, Z
	{ 0,  1,  0},
	{ 0,  0,  1},
	{ 0,  1,  0},
};

// TODO: minimize duplicated code with drawReflectionProbe
void grendx::drawShadowCubeMap(renderQueue& queue,
                               sceneLightPoint::ptr light,
                               glm::mat4& transform,
                               renderContext::ptr rctx)
{
	if (!light->casts_shadows) {
		return;
	}

	// refresh atlas cache time to indicate it was recently used
	for (unsigned i = 0; i < 6; i++) {
		light->shadowmap[i] =
			rctx->atlases.shadows->tree.refresh(light->shadowmap[i]);
	}

	if (light->is_static && light->have_map) {
		// static probe already rendered, nothing to do
		return;
	}

	profile::startGroup("shadow cubemap : " + light->idString());

	enable(GL_SCISSOR_TEST);
	enable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);
	glDepthFunc(GL_LESS);

	camera::ptr cam = camera::ptr(new camera());
	cam->setPosition(applyTransform(transform));
	cam->setFovx(90);
	cam->setFar(50);

	renderFlags flags = rctx->probeShaders["shadow"];
	flags.features |= renderFlags::Features::Shadowmap;

	// TODO: 'cull radius' function for queues to avoid reprocessing the
	//       entire render queue 6 times
	// TODO: could also split queue culling into multiple threads, if it's
	//       still a problem after culling by radius
	for (unsigned i = 0; i < 6; i++) {
		if (!rctx->atlases.shadows->bind_atlas_fb(light->shadowmap[i])) {
			std::cerr
				<< "drawShadowCubeMap(): couldn't bind shadow framebuffer"
				<< std::endl;
			continue;
		}

		// TODO: this might result in shader recompilation?
		//       need all clear bits I think, test later
		glClear(GL_DEPTH_BUFFER_BIT);

		renderQueue porque = queue;
		// TODO: texture atlas should have some tree wrappers, just for
		//       clean encapsulation...
		quadinfo info = rctx->atlases.shadows->tree.info(light->shadowmap[i]);

		cam->setDirection(cube_dirs[i], cube_up[i]);
		cam->setViewport(info.size, info.size);

		cullQueue(porque, cam, info.size, info.size, rctx->lightThreshold);
		sortQueue(porque, cam);

		flush(porque, cam, info.size, info.size, rctx, flags);
	}

	light->have_map = true;
	profile::endGroup();
}

// TODO: minimize duplicated code with drawReflectionProbe
void grendx::drawSpotlightShadow(renderQueue& queue,
                                 sceneLightSpot::ptr light,
                                 glm::mat4& transform,
                                 renderContext::ptr rctx)
{
	if (!light->casts_shadows) {
		return;
	}

	// refresh atlas cache time to indicate it was recently used
	light->shadowmap = rctx->atlases.shadows->tree.refresh(light->shadowmap);

	if (light->is_static && light->have_map) {
		// static probe already rendered, nothing to do
		return;
	}

	profile::startGroup(light->idString());

	enable(GL_SCISSOR_TEST);
	enable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);
	glDepthFunc(GL_LESS);

	camera::ptr cam = camera::ptr(new camera());
	profile::startGroup("Set flags, bind");
	cam->setPosition(applyTransform(transform));
	//cam->setFovx(360.f*acos(light->angle)/M_PI);
	cam->setFovx(2.f*180.f*acosf(light->angle)/M_PI);
	//cam->setFovx(90);
	// TODO: configurable/dynamic far plane
	//cam->setFar(light->extent(rctx->lightThreshold));
	cam->setFar(50);

	renderFlags flags = rctx->probeShaders["shadow"];
	flags.features |= renderFlags::Features::Shadowmap;

	if (!rctx->atlases.shadows->bind_atlas_fb(light->shadowmap)) {
		std::cerr
			<< "drawSpotlightShadow(): couldn't bind shadow framebuffer"
			<< std::endl;
		return;
	}
	profile::endGroup();

	profile::startGroup("Clear");
	// TODO: this might result in shader recompilation?
	glClear(GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);
	profile::endGroup();

	profile::startGroup("Build queue");
	renderQueue porque = queue;
	// TODO: texture atlas should have some tree wrappers, just for
	//       clean encapsulation...
	quadinfo info = rctx->atlases.shadows->tree.info(light->shadowmap);
	glm::vec3 dir = glm::mat3(transform) * glm::vec3(0, 0, -1);
	glm::vec3 up  = glm::mat3(transform) * glm::vec3(0, 1, 0);

	cam->setDirection(dir, up);
	cam->setViewport(info.size, info.size);
	profile::endGroup();

	profile::startGroup("Cull + Sort");
	cullQueue(porque, cam, info.size, info.size, rctx->lightThreshold);
	sortQueue(porque, cam);
	renderQueue& newque = porque;
	profile::endGroup();

	profile::startGroup("Draw");
	flush(newque, cam, info.size, info.size, rctx, flags);
	profile::endGroup();
	DO_ERROR_CHECK();

	light->have_map = true;
	light->shadowproj = cam->viewProjTransform();
	profile::endGroup();
}

static void convoluteReflectionProbeMips(sceneReflectionProbe::ptr probe,
                                         renderContext::ptr rctx,
                                         unsigned level)
{
	Program::ptr convolve = rctx->postShaders["specular-convolve"];
	convolve->bind();

	std::cout << "convolve level " << level << std::endl;

	glActiveTexture(TEX_GL_REFLECTIONS);
	rctx->atlases.reflections->color_tex->bind();
	convolve->set("reflection_atlas", TEXU_REFLECTIONS);
	DO_ERROR_CHECK();

	for (unsigned i = 0; i < 6; i++) {
		glm::vec3 facevec =
			rctx->atlases.reflections->tex_vector(probe->faces[0][i]);
		std::string sloc = "cubeface["+std::to_string(i)+"]";
		convolve->set(sloc, facevec);
	}

	for (unsigned i = 0; i < 6; i++) {
		// note: irradiance, not reflection atlas
		probe->faces[level][i] =
			rctx->atlases.irradiance->tree.refresh(probe->faces[level][i]);
	}

	for (unsigned i = 0; i < 6; i++) {
		quadinfo info = rctx->atlases.irradiance->tree.info(probe->faces[level][i]);
		quadinfo sourceinfo =
			rctx->atlases.reflections->tree.info(probe->faces[0][i]);

		if (!rctx->atlases.irradiance->bind_atlas_fb(probe->faces[level][i])) {
			std::cerr
				<< "convoluteReflectionProbeMips(): couldn't bind probe framebuffer"
				<< std::endl;
			continue;
		}

		bindVao(getScreenquadVao());
		glClearColor(0.0, 0.0, 0.0, 1.0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glDepthMask(GL_FALSE);
		disable(GL_DEPTH_TEST);
		DO_ERROR_CHECK();

		convolve->set("currentFace", int(i));
		convolve->set("screen_x", float(info.size));
		convolve->set("screen_y", float(info.size));
		convolve->set("rend_x", float(sourceinfo.size));
		convolve->set("rend_y", float(sourceinfo.size));
		convolve->set("exposure", 1.f);

		drawScreenquad();
	}
}

void grendx::drawReflectionProbe(renderQueue& queue,
                                 sceneReflectionProbe::ptr probe,
                                 glm::mat4& transform,
                                 renderContext::ptr rctx)
{
	// refresh top level reflection probe to keep/reallocate it
	for (unsigned i = 0; i < 6; i++) {
		probe->faces[0][i] =
			rctx->atlases.reflections->tree.refresh(probe->faces[0][i]);
	}

	if (probe->is_static && probe->have_map) {
		// static probe already rendered, nothing to do
		return;
	}

	// TODO: check for updates inside some radius, return if none

	enable(GL_SCISSOR_TEST);
	enable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);
	glDepthFunc(GL_LESS);
	DO_ERROR_CHECK();

	camera::ptr cam = camera::ptr(new camera());
	cam->setPosition(applyTransform(transform));
	cam->setFovx(90);

	const renderFlags& flags = rctx->probeShaders["refprobe"];

	glActiveTexture(TEX_GL_SHADOWS);
	rctx->atlases.shadows->depth_tex->bind();

	for (Program::ptr prog : {flags.mainShader,
	                          flags.skinnedShader,
	                          flags.instancedShader})
	{
		prog->bind();
		shaderSync(prog, rctx, queue);
		prog->set("shadowmap_atlas", TEXU_SHADOWS);
	}

	DO_ERROR_CHECK();

	for (unsigned i = 0; i < 6; i++) {
		if (!rctx->atlases.reflections->bind_atlas_fb(probe->faces[0][i])) {
			std::cerr
				<< "drawReflectionProbe(): couldn't bind probe framebuffer"
				<< std::endl;
			continue;
		}

		glClearColor(0.0, 0.0, 0.0, 1.0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		renderQueue porque = queue;
		quadinfo info = rctx->atlases.reflections->tree.info(probe->faces[0][i]);
		cam->setDirection(cube_dirs[i], cube_up[i]);
		cam->setViewport(info.size, info.size);
		DO_ERROR_CHECK();

		flush(porque, cam, info.size, info.size, rctx, flags);
		DO_ERROR_CHECK();

		rctx->defaultSkybox.draw(cam, info.size, info.size);
		DO_ERROR_CHECK();
	}

	if (probe->have_convolved) {
		for (unsigned k = 1; k < 5; k++) {
			convoluteReflectionProbeMips(probe, rctx, k);
		}
	}

	probe->have_map = true;
}

void grendx::drawIrradianceProbe(renderQueue& queue,
                                 sceneIrradianceProbe::ptr probe,
                                 glm::mat4& transform,
                                 renderContext::ptr rctx)
{
	// XXX
	probe->source->setTransform(probe->getTransformTRS());
	probe->source->is_static = probe->is_static;
	probe->source->have_map  = probe->have_map;
	probe->source->changed   = probe->changed;
	drawReflectionProbe(queue, probe->source, transform, rctx);

	if (probe->is_static && probe->have_map) {
		// static probe already rendered, nothing to do
		return;
	}

	for (unsigned i = 0; i < 6; i++) {
		probe->faces[i] =
			rctx->atlases.irradiance->tree.refresh(probe->faces[i]);
		// TODO: coefficients, spherical harmonics
	}

	// TODO: check for updates inside some radius, return if none
	Program::ptr convolve = rctx->postShaders["irradiance-convolve"];
	convolve->bind();

	glActiveTexture(TEX_GL_REFLECTIONS);
	rctx->atlases.reflections->color_tex->bind();
	convolve->set("reflection_atlas", TEXU_REFLECTIONS);
	DO_ERROR_CHECK();

	for (unsigned i = 0; i < 6; i++) {
		glm::vec3 facevec =
			rctx->atlases.reflections->tex_vector(probe->source->faces[0][i]);
		std::string sloc = "cubeface["+std::to_string(i)+"]";
		convolve->set(sloc, facevec);
	}

	for (unsigned i = 0; i < 6; i++) {
		quadinfo info = rctx->atlases.irradiance->tree.info(probe->faces[i]);
		quadinfo sourceinfo =
			rctx->atlases.reflections->tree.info(probe->source->faces[0][i]);

		if (!rctx->atlases.irradiance->bind_atlas_fb(probe->faces[i])) {
			std::cerr
				<< "drawIrradianceProbe(): couldn't bind probe framebuffer"
				<< std::endl;
			continue;
		}

		bindVao(getScreenquadVao());
		glClearColor(0.0, 0.0, 0.0, 1.0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glDepthMask(GL_FALSE);
		disable(GL_DEPTH_TEST);
		DO_ERROR_CHECK();

		convolve->set("currentFace", int(i));
		convolve->set("screen_x", float(info.size));
		convolve->set("screen_y", float(info.size));
		convolve->set("rend_x", float(sourceinfo.size));
		convolve->set("rend_y", float(sourceinfo.size));
		convolve->set("exposure", 1.f);

		drawScreenquad();
	}

	puts("rendered irradiance probe");
	probe->have_map = true;
}
