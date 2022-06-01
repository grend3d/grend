#pragma once

#include <grend/glmIncludes.hpp>
#include <grend/glManager.hpp>

#include <variant>
#include <map>
#include <string>

namespace grendx {

typedef std::map<std::string, Shader::parameters> shaderOptions;

std::string preprocessShader(std::string& source,
                             const Shader::parameters& opts);

// namespace grendx;
}
