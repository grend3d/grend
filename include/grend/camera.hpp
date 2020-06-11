#pragma once
#include <grend/glm-includes.hpp>

namespace grendx {

class camera {
	public:
		glm::vec3 position = glm::vec3(0);
		glm::vec3 velocity = glm::vec3(0);
		glm::vec3 direction = glm::vec3(0, 0, 1);

		glm::vec3 right = glm::vec3(1, 0, 0);
		glm::vec3 up = glm::vec3(0, 1, 0);

		void set_direction(glm::vec3 dir);
};

// namespace grendx
}
