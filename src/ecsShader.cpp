#include <grend/ecs/shader.hpp>
#include <grend/renderContext.hpp>

namespace grendx::ecs {

abstractShader::~abstractShader() {};
staticShader::~staticShader() {};
PBRShader::~PBRShader() {};
UnlitShader::~UnlitShader() {};

abstractShader::abstractShader(regArgs t)
	: component(doRegister(this, t)) {}

staticShader::staticShader(regArgs t, const renderFlags& flags)
	: abstractShader(doRegister(this, t)),
	  shader(flags) { };

renderFlags& staticShader::getShader() {
	return shader;
}

PBRShader::PBRShader(regArgs t)
	// TODO: reference PBR shader somehow, don't load internally inside the engine
	: staticShader(doRegister(this, t),
	               // hmmmm... little bit gross
	               t.manager->engine->services.resolve<renderContext>()->getLightingFlags()) {}

UnlitShader::UnlitShader(regArgs t)
	// TODO: reference unshaded shader somehow, don't load internally inside the engine
	: staticShader(doRegister(this, t),
	               t.manager->engine->services.resolve<renderContext>()->getLightingFlags("unshaded")) {}

// namespace grendx::ecs
}
