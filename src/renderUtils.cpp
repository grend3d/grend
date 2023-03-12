#include <grend/renderUtils.hpp>
#include <grend/engine.hpp>

using namespace grendx;
using namespace grendx::engine;

void grendx::renderWorld(camera::ptr cam, renderFlags& flags) {
	renderQueue que;
	renderWorld(cam, que, flags);
}

void grendx::renderWorld(camera::ptr cam,
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
void grendx::drawMultiQueue(multiRenderQueue& que,
                            renderFramebuffer::ptr fb,
                            camera::ptr cam)
{
	auto rend = Resolve<renderContext>();

	// XXX: less than ideal
	// TODO: need a better way to update global lighting state,
	//       without (or minimizing) allocations, this could add a lot
	//       of pointless overhead
	renderQueue hax;
	for (auto& [id, que] : que.queues) {
		hax.add(que);
	}

	updateLights(rend, hax);
	updateReflections(rend, hax);
	buildTilemap(hax.lights, cam, rend);
	updateReflectionProbe(rend, hax, cam);

	// TODO: better config
#define DEPTH_PASS 1
#if DEPTH_PASS
	renderOptions depthOpts, regOpts;
	depthOpts.features |= renderOptions::Shadowmap;
	regOpts.depthFunc = renderOptions::Equal;
	// no need to write depth
	regOpts.features &= ~(renderOptions::DepthMask);

	// depth pass
	renderFlags flags = rend->probeShaders["shadow"];
	cullQueue(hax, cam,
			  rend->framebuffer->width,
			  rend->framebuffer->height,
			  rend->lightThreshold);
	sortQueue(hax, cam);

	rend->framebuffer->setOutputEnabled(false);
	// don't count prepass as drawn meshes
	flush(hax, cam, fb, rend, flags, depthOpts);
	rend->framebuffer->setOutputEnabled(true);
#else
	renderOptions regOpts;
	regOpts.depthFunc = renderOptions::Less;
#endif

	// lighting pass
	cullQueue(que, cam,
			  rend->framebuffer->width,
			  rend->framebuffer->height,
			  rend->lightThreshold);
	sortQueue(que, cam);

	// TODO: profile: reimplement this
	//game->metrics.drawnMeshes += flush(que, cam, fb, rend, regOpts);
	flush(que, cam, fb, rend, regOpts);
}

#include <grend/ecs/shader.hpp>
#include <grend/ecs/sceneComponent.hpp>

grendx::multiRenderQueue
grendx::buildDrawableQueue(uint32_t renderID) {
	using namespace ecs;
	auto entities = Resolve<entityManager>();

	multiRenderQueue que;

	for (auto [ent, flags] : entities->search<abstractShader>()) {
		for (auto scene : ent->getAll<sceneComponent>()) {
			que.add(flags->getShader(),
			        scene->getNode(),
			        renderID,
			        ent->transform.getTransform());
		}
	}

	return que;
}

// TODO: should return the last allocated render ID too, or take a counter pointer
void
grendx::buildClickableQueue(clickableEntities& clicks, multiRenderQueue& que) {
	using namespace grendx;
	using namespace ecs;

	auto entities = Resolve<ecs::entityManager>();

	for (auto [ent, flags] : entities->search<abstractShader>()) {
		renderFlags shader = flags->getShader();
		uint32_t renderID = clicks.add(ent);

		for (auto scene : ent->getAll<sceneComponent>()) {
			que.add(shader,
			        scene->getNode(),
			        renderID,
			        ent->transform.getTransform());
		}
	};
}

static
void buildClickableRec(clickableEntities& clicks,
                       sceneNode::ptr obj,
                       renderQueue& que,
                       uint32_t renderID,
                       glm::mat4 trans,
                       bool inverted)
{
	if (!obj) return;

	glm::mat4 adjTrans;
	bool adjInverted;

	getNodeTransform(obj, trans, inverted, adjTrans, adjInverted);

	if (obj->type == sceneNode::objType::Import) {
		renderID = clicks.add(obj.getPtr());
	}

	if (que.addNode(obj, renderID, adjTrans, adjInverted)) {
		for (auto ptr : obj->nodes()) {
			buildClickableRec(clicks,
			                  ptr->getRef(),
			                  que,
			                  renderID,
			                  adjTrans,
			                  adjInverted);
		}
	}
}

void grendx::buildClickableImports(clickableEntities& clicks,
                                   sceneNode::ptr obj,
                                   renderQueue& que)
{
	buildClickableRec(clicks, obj, que, 0, glm::mat4(1), false);
}

void grendx::setPostUniforms(renderPostChain::ptr post,
                             camera::ptr cam)
{
	auto rend = Resolve<renderContext>();

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
	post->setUniform("exposure", rend->exposure);
	post->setUniform("time_ms",  tim);
	post->setUniform("lightThreshold", rend->lightThreshold);

	// TODO: bind texture
	post->setUniform("shadowmap_atlas", TEXU_SHADOWS);

	post->setUniformBlock("lights", rend->lightBuffer, UBO_LIGHT_INFO);
	post->setUniformBlock("point_light_tiles", rend->pointTiles,
						  UBO_POINT_LIGHT_TILES);
	post->setUniformBlock("spot_light_tiles", rend->spotTiles,
						  UBO_SPOT_LIGHT_TILES);
	post->setUniformBlock("point_light_buffer", rend->pointBuffer, UBO_POINT_LIGHT_BUFFER);
	post->setUniformBlock("spot_light_buffer", rend->spotBuffer, UBO_SPOT_LIGHT_BUFFER);
	post->setUniformBlock("directional_light_buffer", rend->directionalBuffer, UBO_DIRECTIONAL_LIGHT_BUFFER);
}

clickableEntities::clickableEntities(uint32_t reserved)
	: reservedStart(reserved) {};

uint32_t clickableEntities::add(ecs::entity* ent) {
	size_t temp = clickmap.size();
	clickmap.push_back(ent);

	return reservedStart + temp;
}

ecs::entity* clickableEntities::get(uint32_t id) {
	if (id >= reservedStart && id - reservedStart < clickmap.size()) {
		return clickmap[id - reservedStart];
	}

	return nullptr;
}

void clickableEntities::clear(void) {
	clickmap.clear();
}

size_t clickableEntities::size(void) {
	return clickmap.size();
}
