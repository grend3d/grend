#include <grend/ecs/shader.hpp>

namespace grendx::ecs {

abstractShader::~abstractShader() {};
staticShader::~staticShader() {};
PBRShader::~PBRShader() {};
UnlitShader::~UnlitShader() {};

abstractShader::abstractShader(entityManager *manager, entity *ent)
	: component(manager, ent)
{
	manager->registerComponent(ent, this);
}

staticShader::staticShader(entityManager *manager, entity *ent, const renderFlags& flags)
	: abstractShader(manager, ent),
	  shader(flags)
{
	manager->registerComponent(ent, this);
};

renderFlags& staticShader::getShader() {
	return shader;
}

PBRShader::PBRShader(entityManager *manager, entity *ent)
	// TODO: reference PBR shader somehow, don't load internally inside the engine
	: staticShader(manager, ent, manager->engine->rend->getLightingFlags())
{
	manager->registerComponent(ent, this);
}

UnlitShader::UnlitShader(entityManager *manager, entity *ent)
	// TODO: reference unshaded shader somehow, don't load internally inside the engine
	: staticShader(manager, ent, manager->engine->rend->getLightingFlags("unshaded"))
{
	manager->registerComponent(ent, this);
}

// namespace grendx::ecs
}
