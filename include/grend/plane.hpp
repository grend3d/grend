#pragma once
#include <grend/glmIncludes.hpp>

namespace grendx {

struct plane {
	glm::vec3 n;
	float d;
	bool inPlane(glm::vec3 pos, float radius=0.0);
	float distance(glm::vec3 pos);
};

// namespace grendx
};
