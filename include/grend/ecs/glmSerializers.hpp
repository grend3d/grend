#pragma once

#include <grend/ecs/bufferComponent.hpp>
#include <grend/glmIncludes.hpp>

namespace grendx::ecs {

template <typename T>
concept glmVectorType =
	PrimitiveType<typename T::value_type>
	&& requires () { T::length; };

template <glmVectorType T>
struct vectorSerializer {
	static void serializeBytes(T* val, uint8_t *buf, size_t offset) {
		auto serializer = getSerializerImpl<typename T::value_type>();
		auto size = serializer.size();

		for (size_t i = 0; i < (size_t)T::length(); i++) {
			serializer.serialize(glm::value_ptr(*val) + i, buf, offset + i*size);
		}
	}

	static void deserializeBytes(T* val, uint8_t *buf, size_t offset) {
		auto serializer = getSerializerImpl<typename T::value_type>();
		auto size = serializer.size();

		for (size_t i = 0; i < (size_t)T::length(); i++) {
			serializer.deserialize(glm::value_ptr(*val) + i, buf, offset + i*size);
		}
	}

	static size_t serializedByteSize(void) {
		auto serializer = getSerializerImpl<typename T::value_type>();
		return serializer.size() * T::length();
	}
};

template <> struct primitiveSerializer<glm::vec2> : vectorSerializer<glm::vec2> {};
template <> struct primitiveSerializer<glm::vec3> : vectorSerializer<glm::vec3> {};
template <> struct primitiveSerializer<glm::vec4> : vectorSerializer<glm::vec4> {};

// namespace grendx
}
