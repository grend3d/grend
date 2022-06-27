#include <grend/gameMain.hpp>
#include <grend/renderPostStage.hpp>
#include <grend/camera.hpp>

#include <grend/camera.hpp>
#include <grend/renderQueue.hpp>

namespace grendx {

// TODO: move to seperate header
// common world-drawing function
void renderWorld(gameMain *game, camera::ptr cam, renderFlags& flags);
void renderWorld(gameMain *game, camera::ptr cam,
                 renderQueue& base, renderFlags& flags);

// TODO: Maybe replaces drawWorld?
void drawMultiQueue(gameMain *game, multiRenderQueue& que,
                    renderFramebuffer::ptr fb, camera::ptr cam);

// TODO: Maybe add to an existing multiRenderQueue?
//       or could have an overload for that
// TODO: better place for this
multiRenderQueue buildDrawableQueue(gameMain *game, camera::ptr cam);

void setPostUniforms(renderPostChain::ptr post,
                     gameMain *game,
                     camera::ptr cam);

// namespace grendx
}
