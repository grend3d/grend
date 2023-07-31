#pragma once

#include <grend/ecs/ecs.hpp>
#include <grend/material.hpp>

namespace grendx::ecs {

class materialComponent : public component {
	public:
		materialComponent(regArgs t)
			: component(doRegister(this, t)) {};

		materialComponent(regArgs t, const material& m)
			: component(doRegister(this, t)),
			  mat(m) {};

		material mat;

		// TODO:
		static nlohmann::json serializer(component *comp) { return {}; }
		static void deserializer(component *comp, nlohmann::json j) { };
};

// namespace grendx::ecs
}
