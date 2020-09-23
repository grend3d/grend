#pragma once

#include <grend/gameMain.hpp>
#include <grend/gameObject.hpp>
#include <grend/modalSDLInput.hpp>
#include <grend/camera.hpp>

namespace grendx {
namespace controller {

// TODO: maybe make this a template, could be applied to
//       anything with a position field
bindFunc camMovement(camera::ptr cam, float accel);
bindFunc camFPS(camera::ptr cam, gameMain *game);
bindFunc camFocus(camera::ptr cam, gameObject::ptr focus);

// namespace controller
}
// namespace grendx
}
