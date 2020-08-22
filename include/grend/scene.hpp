#pragma once

#include <grend/glm-includes.hpp>
#include <vector>
#include <string>

namespace grendx {

class scene {
	public:
		struct node {
			std::string name;
			glm::vec3 position = glm::vec3(0);
			glm::vec3 scale    = glm::vec3(1);
			glm::quat rotation = glm::quat(1, 0, 0, 0);
		};

		std::vector<struct node> nodes;
};

// namespace grendx
}
