#pragma once

#include <grend/gameMain.hpp>
#include <grend/sceneNode.hpp>
#include <grend/modalSDLInput.hpp>
#include <grend/camera.hpp>
#include <grend/renderPostStage.hpp>

namespace grendx {
namespace controller {

// TODO: maybe make this a template, could be applied to
//       anything with a position field
bindFunc camMovement(camera::ptr cam, float accel);
bindFunc camMovement2D(camera::ptr cam, float accel);
bindFunc camFPS(camera::ptr cam, gameMain *game);
bindFunc camAngled2D(camera::ptr cam, gameMain *game, float angle);
bindFunc camAngled2DFixed(camera::ptr cam, gameMain *game, float angle);
bindFunc camAngled2DRotatable(camera::ptr cam,
		gameMain *game,
		float angle,
		// limit degrees of rotation for up/down movement, default is unrestricted
		float minY = -M_PI,
		float maxY =  M_PI);
bindFunc camFocus(camera::ptr cam, sceneNode::ptr focus);
bindFunc camScrollZoom(camera::ptr cam, float *zoom, float scale=1.f);

// namespace controller
}

// XXX: don't know where to put this, input handlers for
//      resizing the framebuffer on window resize, etc.
// TODO: maybe dedicated header
bindFunc resizeInputHandler(gameMain *game, renderPostChain::ptr post);

// namespace grendx
}
