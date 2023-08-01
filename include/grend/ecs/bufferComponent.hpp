#pragma once

#include <grend/ecs/ecs.hpp>

namespace grendx::ecs {

template <typename T>
class bufferComponent : public component {
	public:
		bufferComponent(regArgs t)
			: component(doRegister(this, t)) {};

		std::vector<T> data;

		// TODO:
		static nlohmann::json serializer(component *comp) { return {}; }
		static void deserializer(component *comp, nlohmann::json j) { };
};

// namespace grendx::ecs
}
