#pragma once
#include <grend/glm-includes.hpp>

namespace grendx {

struct TRS {
	glm::vec3 position = glm::vec3(0, 0, 0);
	glm::quat rotation = glm::quat(1, 0, 0, 0);
	glm::vec3 scale    = glm::vec3(1, 1, 1);

	glm::mat4 getTransform(void) {
		return glm::translate(position)
			* glm::mat4_cast(rotation)
			* glm::scale(scale);
	}
};

// namespace grendx
}
