#include <grend/renderUtils.hpp>

using namespace grendx;

void grendx::renderWorld(gameMain *game, camera::ptr cam, renderFlags& flags) {
	renderQueue que;
	renderWorld(game, cam, que, flags);
}

void grendx::renderWorld(gameMain *game,
                         camera::ptr cam,
                         renderQueue& base,
                         renderFlags& flags)
{
#if 0
	profile::startGroup("renderWorld()");
	assert(game->rend && game->rend->framebuffer);

	int winsize_x, winsize_y;
	SDL_GetWindowSize(game->ctx.window, &winsize_x, &winsize_y);

	if (game->state->rootnode) {
		renderQueue que(base);
		//que.setCamera(cam);
		float fticks = SDL_GetTicks() / 1000.0f;

		DO_ERROR_CHECK();

		profile::startGroup("Build queue");
		que.add(game->state->rootnode);
		que.add(game->state->physObjects);

		profile::startGroup("Update lights/shadowmaps");
		updateLights(game->rend, que);
		updateReflections(game->rend, que);
		profile::endGroup();
		profile::endGroup();
		DO_ERROR_CHECK();

		game->rend->framebuffer->bind();
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		DO_ERROR_CHECK();

		flags.mainShader->bind();
		flags.mainShader->set("time_ms", SDL_GetTicks() * 1.f);
		DO_ERROR_CHECK();

		profile::startGroup("Cull");
		cullQueue(que, cam, game->rend->framebuffer->width,
		          game->rend->framebuffer->height,
		          game->rend->lightThreshold);
		renderQueue& newque = que;
		profile::endGroup();

		profile::startGroup("Sort");
		//que.sort();
		sortQueue(newque, cam);
		profile::endGroup();

		//profile::startGroup("Batch");
		//que.batch();
		//profile::endGroup();

		profile::startGroup("Build light tilemap");
		buildTilemap(newque.lights, cam, game->rend);
		profile::endGroup();

		profile::startGroup("Draw");
		updateReflectionProbe(game->rend, newque, cam);
		game->metrics.drawnMeshes +=
			flush(newque, cam, game->rend->framebuffer, game->rend, flags);
		DO_ERROR_CHECK();
		game->rend->defaultSkybox.draw(cam, game->rend->framebuffer);
		DO_ERROR_CHECK();
		profile::endGroup();
	}

	profile::endGroup();
#endif
}

// TODO: Maybe replaces drawWorld?
void grendx::drawMultiQueue(gameMain *game,
                            multiRenderQueue& que,
                            renderFramebuffer::ptr fb,
                            camera::ptr cam)
{
	// XXX: less than ideal
	// TODO: need a better way to update global lighting state,
	//       without (or minimizing) allocations, this could add a lot
	//       of pointless overhead
	renderQueue hax;
	for (auto& [id, que] : que.queues) {
		hax.add(que);
	}

	updateLights(game->rend, hax);
	updateReflections(game->rend, hax);
	buildTilemap(hax.lights, cam, game->rend);
	updateReflectionProbe(game->rend, hax, cam);

	// TODO: better config
#define DEPTH_PASS 1
#if DEPTH_PASS
	renderOptions depthOpts, regOpts;
	depthOpts.features |= renderOptions::Shadowmap;
	regOpts.depthFunc = renderOptions::Equal;
	// no need to write depth
	regOpts.features &= ~(renderOptions::DepthMask);

	// depth pass
	renderFlags flags = game->rend->probeShaders["shadow"];
	cullQueue(hax, cam,
			  game->rend->framebuffer->width,
			  game->rend->framebuffer->height,
			  game->rend->lightThreshold);
	sortQueue(hax, cam);

	// don't count prepass as drawn meshes
	flush(hax, cam, fb, game->rend, flags, depthOpts);
#else
	renderOptions regOpts;
	regOpts.depthFunc = renderOptions::Less;
#endif

	// lighting pass
	cullQueue(que, cam,
			  game->rend->framebuffer->width,
			  game->rend->framebuffer->height,
			  game->rend->lightThreshold);
	sortQueue(que, cam);
	game->metrics.drawnMeshes += flush(que, cam, fb, game->rend, regOpts);
}

#include <grend/ecs/shader.hpp>
#include <grend/ecs/sceneComponent.hpp>

grendx::multiRenderQueue grendx::buildDrawableQueue(gameMain *game, camera::ptr cam) {
	using namespace ecs;
	//auto drawable = searchEntities(game->entities.get(), {getTypeName<abstractShader>()});
	auto drawable = game->entities->search<abstractShader>();

	multiRenderQueue que;

	for (entity *ent : drawable) {
		auto flags = ent->get<abstractShader>();
		auto trans = ent->node->getTransformMatrix();
		auto scenes = ent->getAll<sceneComponent>();
		// TODO: do clicky things
		uint32_t renderID = 0;

		que.add(flags->getShader(), ent->node, renderID);

		for (auto it = scenes.first; it != scenes.second; it++) {
			sceneComponent *comp = static_cast<sceneComponent*>(it->second);
			que.add(flags->getShader(), comp->getNode(), renderID, trans);
		}
	}

	return que;
}

void grendx::setPostUniforms(renderPostChain::ptr post,
                             gameMain *game,
                             camera::ptr cam)
{
	glm::mat4 view       = cam->viewTransform();
	glm::mat4 projection = cam->projectionTransform();
	glm::mat4 v_inv      = glm::inverse(view);

	post->setUniform("v", view);
	post->setUniform("p", projection);
	post->setUniform("v_inv", v_inv);

	post->setUniform("cameraPosition", cam->position());
	post->setUniform("cameraForward",  cam->direction());
	post->setUniform("cameraRight",    cam->right());
	post->setUniform("cameraUp",       cam->up());
	post->setUniform("cameraFov",      cam->fovx());

	float tim = SDL_GetTicks() * 1000.f;
	post->setUniform("exposure", game->rend->exposure);
	post->setUniform("time_ms",  tim);
	post->setUniform("lightThreshold", game->rend->lightThreshold);

	// TODO: bind texture
	post->setUniform("shadowmap_atlas", TEXU_SHADOWS);

	post->setUniformBlock("lights", game->rend->lightBuffer, UBO_LIGHT_INFO);
	post->setUniformBlock("point_light_tiles", game->rend->pointTiles,
						  UBO_POINT_LIGHT_TILES);
	post->setUniformBlock("spot_light_tiles", game->rend->spotTiles,
						  UBO_SPOT_LIGHT_TILES);
	post->setUniformBlock("point_light_buffer", game->rend->pointBuffer, UBO_POINT_LIGHT_BUFFER);
	post->setUniformBlock("spot_light_buffer", game->rend->spotBuffer, UBO_SPOT_LIGHT_BUFFER);
	post->setUniformBlock("directional_light_buffer", game->rend->directionalBuffer, UBO_DIRECTIONAL_LIGHT_BUFFER);
}
