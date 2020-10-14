#pragma once
#include <grend/glm-includes.hpp>

namespace grendx {

struct AABB {
	glm::vec3 min;
	glm::vec3 max;
};

static inline struct AABB operator*(glm::mat4 m, struct AABB& a) {
	glm::vec4 amin = m*glm::vec4(a.min, 1);
	glm::vec4 amax = m*glm::vec4(a.max, 1);

	return (struct AABB) {
		.min = glm::vec3(amin)/amin.w,
		.max = glm::vec3(amax)/amax.w,
	};
}

// namespace grendx
}
