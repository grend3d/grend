#include <grend/shaderPreprocess.hpp>
#include <grend/glManager.hpp>
#include <grend/engine.hpp>

using namespace grendx;

std::string grendx::preprocessShader(std::string& source, shaderOptions& opts) {
	std::string version = std::string("#version ") + GLSL_STRING + "\n";

	std::string defines =
		"#define MAX_LIGHTS " + std::to_string(MAX_LIGHTS) + "\n" +
		"#define GLSL_VERSION " + std::to_string(GLSL_VERSION) + "\n" +
		"\n";

	return version + defines + source;
}
