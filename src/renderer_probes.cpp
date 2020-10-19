#include <grend/engine.hpp>
#include <grend/gameModel.hpp>
#include <grend/utility.hpp>
#include <grend/texture-atlas.hpp>

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
                               gameLightPoint::ptr light,
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

	enable(GL_SCISSOR_TEST);
	enable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);
	glDepthFunc(GL_LESS);

	camera::ptr cam = camera::ptr(new camera());
	Program::ptr shadowShader = rctx->shaders["shadow"];

	shadowShader->bind();
	queue.shaderSync(shadowShader, rctx);
	cam->setPosition(light->transform.position);
	cam->setFovx(90);

	// TODO: skinned shadow shader
	renderFlags flags = rctx->getDefaultFlags("shadow");
	rctx->setFlags(flags);

	for (unsigned i = 0; i < 6; i++) {
		if (!rctx->atlases.shadows->bind_atlas_fb(light->shadowmap[i])) {
			std::cerr
				<< "drawShadowCubeMap(): couldn't bind shadow framebuffer"
				<< std::endl;
			continue;
		}

		glClear(GL_DEPTH_BUFFER_BIT);

		renderQueue porque = queue;
		// TODO: texture atlas should have some tree wrappers, just for
		//       clean encapsulation...
		quadinfo info = rctx->atlases.shadows->tree.info(light->shadowmap[i]);
		cam->setDirection(cube_dirs[i], cube_up[i]);
		cam->setViewport(info.size, info.size);

		porque.setCamera(cam);
		// TODO: skinned shadow shader
		porque.flush(info.size, info.size, rctx);
	}

	light->have_map = true;
}

void grendx::drawReflectionProbe(renderQueue& queue,
                                 gameReflectionProbe::ptr probe,
                                 renderContext::ptr rctx)
{
	for (unsigned i = 0; i < 6; i++) {
		probe->faces[i] =
			rctx->atlases.reflections->tree.refresh(probe->faces[i]);
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
	Program::ptr refShader = rctx->shaders["refprobe"];
	cam->setPosition(probe->transform.position);
	cam->setFovx(90);

	refShader->bind();
	glActiveTexture(GL_TEXTURE7);
	rctx->atlases.shadows->depth_tex->bind();
	refShader->set("shadowmap_atlas", 7);

	queue.shaderSync(refShader, rctx);
	DO_ERROR_CHECK();

	// TODO: skinned reflection shader
	//flags.mainShader = flags.skinnedShader = refShader;
	renderFlags flags = rctx->getDefaultFlags("refprobe");
	rctx->setFlags(flags);

	for (unsigned i = 0; i < 6; i++) {
		refShader->bind();
		if (!rctx->atlases.reflections->bind_atlas_fb(probe->faces[i])) {
			std::cerr
				<< "drawReflectionProbe(): couldn't bind probe framebuffer"
				<< std::endl;
			continue;
		}

		glClearColor(0.0, 0.0, 0.0, 1.0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		renderQueue porque = queue;
		quadinfo info = rctx->atlases.reflections->tree.info(probe->faces[i]);
		cam->setDirection(cube_dirs[i], cube_up[i]);
		cam->setViewport(info.size, info.size);
		DO_ERROR_CHECK();

		porque.setCamera(cam);
		// TODO: skinned reflection shader
		porque.flush(info.size, info.size, rctx);
		DO_ERROR_CHECK();

		rctx->defaultSkybox.draw(cam, info.size, info.size);
		DO_ERROR_CHECK();
	}

	probe->have_map = true;
}
