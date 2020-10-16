#pragma once
#include <grend/glm-includes.hpp>

namespace grendx {

struct AABB {
	glm::vec3 min;
	glm::vec3 max;
};

struct OBB {
	glm::vec3 points[8];
	glm::vec3 center;
};

// return OBB from AABB translation, m*a can rotate, skew, etc, the box,
// resulting in a box that isn't aligned with the original axis.
// this also preserves the volume contained in the box, results in more exact
// culling.
//
// TODO: also a function that generates a new AABB around the rotated AABB
static inline struct OBB operator*(glm::mat4 m, struct AABB& a) {
	struct OBB ret;
	glm::vec3 diff = a.max - a.min;

	for (unsigned i = 0; i < 8; i++) {
		float x = (i&1)? diff.x : 0;
		float y = (i&2)? diff.y : 0;
		float z = (i&4)? diff.z : 0;

		ret.points[i] = a.min + glm::vec3(x, y, z);
	}

	for (unsigned i = 0; i < 8; i++) {
		glm::vec4 adj = m*glm::vec4(ret.points[i], 1);
		ret.points[i] = glm::vec3(adj) / adj.w;
	}

	glm::vec3 vmin = ret.points[0];
	glm::vec3 vmax = ret.points[0];

	for (unsigned i = 0; i < 8; i++) {
		vmin = min(vmin, ret.points[i]);
		vmax = max(vmax, ret.points[i]);
	}
	ret.center = 0.5f*(vmin + vmax);

	return ret;
}

// namespace grendx
}
