#include <grend/renderPostStage.hpp>
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

using entClicks = std::vector<std::pair<uint32_t, grendx::ecs::entity*>>;
multiRenderQueue buildClickableQueue(entClicks& clicks,
                                     uint32_t startID = 10);

void setPostUniforms(renderPostChain::ptr post, camera::ptr cam);

// namespace grendx
}
