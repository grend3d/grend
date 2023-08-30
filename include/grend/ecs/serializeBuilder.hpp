#pragma once

#include <grend/ecs/bufferComponent.hpp>

namespace grendx::ecs {

struct serializerState    { uint8_t *buffer; size_t currentOffset; };
struct deserializerState  { uint8_t *buffer; size_t currentOffset; };
struct serializeSizeState { size_t size; };

template <BinarySerializeableType T>
struct serializerState operator<<(const struct serializerState& state, T& value) {
	auto serializer = getSerializerImpl<T>();

	serializer.serialize(&value, state.buffer, state.currentOffset);

	return {
		.buffer        = state.buffer,
		.currentOffset = state.currentOffset + serializer.size()
	};
}

template <BinarySerializeableType T>
struct deserializerState operator<<(const struct deserializerState& state, T& value) {
	auto serializer = getSerializerImpl<T>();

	serializer.deserialize(&value, state.buffer, state.currentOffset);

	return {
		.buffer        = state.buffer,
		.currentOffset = state.currentOffset + serializer.size()
	};
}

template <BinarySerializeableType T>
struct serializeSizeState operator<<(const struct serializeSizeState& state, T& value) {
	auto serializer = getSerializerImpl<T>();

	return { state.size + serializer.size() };
}

static
struct serializerState serializeBuilder(uint8_t *buffer, size_t offset) {
	return {
		.buffer        = buffer,
		.currentOffset = offset,
	};
}

static
struct deserializerState deserializeBuilder(uint8_t *buffer, size_t offset) {
	return {
		.buffer        = buffer,
		.currentOffset = offset,
	};
}

static
struct serializeSizeState serializeSizeBuilder() {
	return { .size = 0 };
}

// namespace grendx
}
