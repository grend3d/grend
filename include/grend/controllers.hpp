#pragma once

#include <grend/sceneNode.hpp>
#include <grend/modalSDLInput.hpp>
#include <grend/camera.hpp>

namespace grendx {
namespace controller {

// TODO: maybe make this a template, could be applied to
//       anything with a position field
bindFunc camMovement(camera::ptr cam, float accel);
bindFunc camMovement2D(camera::ptr cam, float accel);
bindFunc camFPS(camera::ptr cam);
bindFunc camAngled2D(camera::ptr cam, float angle);
bindFunc camAngled2DFixed(camera::ptr cam, float angle);
bindFunc camAngled2DRotatable(camera::ptr cam,
		float angle,
		// limit degrees of rotation for up/down movement, default is unrestricted
		float minY = -M_PI,
		float maxY =  M_PI);
bindFunc camFocus(camera::ptr cam, sceneNode::ptr focus);
bindFunc camScrollZoom(camera::ptr cam, float *zoom, float scale=1.f);

// namespace controller
}

bindFunc resizeInputHandler();

// namespace grendx
}
