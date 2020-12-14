#pragma once
#include <grend/glmIncludes.hpp>

namespace grendx {

struct TRS {
	glm::vec3 position = glm::vec3(0, 0, 0);
	glm::quat rotation = glm::quat(1, 0, 0, 0);
	glm::vec3 scale    = glm::vec3(1, 1, 1);

	glm::mat4 getTransform(void) const {
		return glm::translate(position)
			* glm::mat4_cast(rotation)
			* glm::scale(scale);
	}
};

static inline struct TRS addTRS(const struct TRS& a, const struct TRS& b) {
	return (TRS) {
		.position = a.position + b.position, // TODO: scale?
		.rotation = a.rotation * b.rotation,
		.scale    = a.scale * b.scale,
	};
}

static inline struct TRS mixtrs(struct TRS& a, struct TRS& b, float amount) {
	struct TRS ret;

	ret.position = glm::mix(a.position, b.position, amount);
	ret.rotation = glm::mix(a.rotation, b.rotation, amount);
	ret.scale    = glm::mix(a.scale, b.scale, amount);

	return ret;
}

// namespace grendx
}
