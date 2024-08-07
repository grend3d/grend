#pragma once

#include <grend/renderPostChain.hpp>
#include <grend/renderPostJoin.hpp>
#include <grend/camera.hpp>

#include <grend/camera.hpp>
#include <grend/renderQueue.hpp>
#include <grend/ecs/ecs.hpp>

namespace grendx {

// TODO: move to seperate header
// common world-drawing function
void renderWorld(camera::ptr cam, renderFlags& flags);
void renderWorld(camera::ptr cam, renderQueue& base, renderFlags& flags);

// TODO: Maybe replaces drawWorld?
void drawMultiQueue(multiRenderQueue& que,
                    renderFramebuffer::ptr fb, camera::ptr cam);

// TODO: Maybe add to an existing multiRenderQueue?
//       or could have an overload for that
// TODO: better place for this

multiRenderQueue buildDrawableQueue(uint32_t renderID = 0);

struct clickableEntities {
	clickableEntities(uint32_t reserved = 64);

	uint32_t add(ecs::entity*);
	ecs::entity* get(uint32_t id);
	void clear(void);
	size_t size();

	std::vector<grendx::ecs::entity*> clickmap;
	uint32_t reservedStart;
};

void buildClickableQueue(clickableEntities& clicks,
                         multiRenderQueue& que);
void buildClickableImports(clickableEntities& clicks, sceneNode::ptr obj, renderQueue& que);

void setPostUniforms(renderPostChain::ptr post, camera::ptr cam);
void setPostUniforms(renderPostJoin::ptr post, camera::ptr cam);

// namespace grendx
}
