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
		abstractShader(regArgs t);
		virtual ~abstractShader();
		virtual const char* typeString(void) const { return getTypeName(*this); };

		// TODO: rename renderFlags to renderShader or something
		// TODO: renderFlags& should be const, need to refactor renderQueue
		//       to use const refs
		virtual renderFlags& getShader() = 0;

		static nlohmann::json serializer(component *comp) { return {}; }
		static void deserializer(component *comp, nlohmann::json j) { };
};

// Returns the same shader each invocation
class staticShader : public abstractShader {
	public:
		staticShader(regArgs t, const renderFlags& flags);
		virtual const char* typeString(void) const { return getTypeName(*this); };

		virtual ~staticShader();
		virtual renderFlags& getShader();

	private:
		renderFlags shader;
};

// Returns the default built-in PBR shader
class PBRShader : public staticShader {
	public:
		PBRShader(regArgs t);

		virtual ~PBRShader();
		virtual const char* typeString(void) const { return getTypeName(*this); };
};

class UnlitShader : public staticShader {
	public:
		UnlitShader(regArgs t);

		virtual ~UnlitShader();
		virtual const char* typeString(void) const { return getTypeName(*this); };
};

// namespace grendx::ecs
}
