#pragma once

#include <grend/glslParser.hpp>

#include <string>
#include <vector>
#include <string_view>
#include <cstdint>

namespace grendx {

struct glslStructSpec {

};

struct glslTypespec {
	std::string qualifier;
	std::string type;
	// TODO: optional?
	glslStructSpec structSpec;
};

struct glslUniform {
	std::string name;
	std::string type;
};

struct glslFunction {
	glslTypespec returnType;
	// TODO: parameters
};

struct glslObject {
	glslObject(std::string_view source);

	std::map<std::string, glslUniform>  uniforms;
	std::map<std::string, glslFunction> functions;

	uint64_t objhash;
};

// namespace grendx
}
