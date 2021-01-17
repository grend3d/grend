#pragma once

#include <grend/glmIncludes.hpp>

#include <variant>
#include <map>
#include <string>

namespace grendx {

// one variant to rule them all
typedef std::variant<
	int, float,
	glm::vec2, glm::vec3, glm::vec4,
	glm::mat2, glm::mat3, glm::mat4
> shaderValue;

// "holds_alternative" is a terrible name, cmon c++ standards people
template <typename T>
static inline bool valueIs(const shaderValue& val) {
	return std::holds_alternative<T>(val);
}

typedef std::map<std::string, shaderValue> shaderOptions;

std::string preprocessShader(std::string& source, shaderOptions& opts);

// namespace grendx;
}
