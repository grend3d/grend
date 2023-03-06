#pragma once

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/ext.hpp>
#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>
#include <glm/gtx/rotate_vector.hpp>
//#include <glm/gtx/matrix_decompose.hpp>

namespace grendx {

static inline glm::vec3 applyTransform(const glm::mat4& m, glm::vec3 v={0, 0, 0}) {
	glm::vec4 apos = m*glm::vec4(v, 1.0);
	return glm::vec3(apos) / apos.w;
}

template <typename T>
T& align(T& value, float snapAmount) {
	value = (snapAmount > 0.0)? glm::round(value / snapAmount)*snapAmount : value;
	return value;
}

// namespace grendx;
}
