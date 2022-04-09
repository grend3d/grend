#pragma once

#include <grend/sceneNode.hpp>
#include <grend/loadScene.hpp>
#include <grend/ecs/ecs.hpp>

namespace grendx::ecs {

// Details the base 'getShader()' function,
// having a function to retrieve the shader allows returning different shaders
// programmatically, based on time, state, distance from camera, etc
class abstractShader : public component {
	public:
		abstractShader(entityManager *manager, entity *ent);
		virtual ~abstractShader();

		// TODO: rename renderFlags to renderShader or something
		// TODO: renderFlags& should be const, need to refactor renderQueue
		//       to use const refs
		virtual renderFlags& getShader() = 0;
};

// Returns the same shader each invocation
class staticShader : public abstractShader {
	public:
		staticShader(entityManager *manager, entity *ent, const renderFlags& flags);

		virtual ~staticShader();
		virtual renderFlags& getShader();

	private:
		renderFlags shader;
};

// Returns the default built-in PBR shader
class PBRShader : public staticShader {
	public:
		PBRShader(entityManager *manager, entity *ent);
		virtual ~PBRShader();
};

class UnlitShader : public staticShader {
	public:
		UnlitShader(entityManager *manager, entity *ent);
		virtual ~UnlitShader();
};

// namespace grendx::ecs
}
