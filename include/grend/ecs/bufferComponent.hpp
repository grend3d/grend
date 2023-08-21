#pragma once

#include <grend/ecs/ecs.hpp>
#include <grend/base64.h>
#include <stdint.h>

namespace grendx::ecs {

template <typename T>
using BinarySerializer   = std::function<void(T *val, uint8_t *buffer, size_t offset)>;

template <typename T>
using BinaryDeserializer = std::function<void(T *val, uint8_t *buffer, size_t offset)>;

template <typename T>
using BinarySerializedSize = std::function<size_t(void)>;

template <typename T>
struct primitiveSerializer { };

template <typename T>
struct simplePrimitiveSerializer {
	static void serializeBytes(T* val, uint8_t *buf, size_t offset) {
		// TODO: endianness
		uint8_t *temp = reinterpret_cast<uint8_t*>(val);

		for (size_t i = 0; i < sizeof(T); i++) {
			buf[offset + i] = temp[i];
		}
	}

	static void deserializeBytes(T* val, uint8_t *buf, size_t offset) {
		// TODO: endianness
		uint8_t *temp = reinterpret_cast<uint8_t*>(val);

		for (size_t i = 0; i < sizeof(T); i++) {
			temp[i] = buf[offset + i];
		}
	}

	static size_t serializedByteSize(void) { return sizeof(T); } // TODO: probably want alignment
};

template <> struct primitiveSerializer<uint8_t>  : simplePrimitiveSerializer<uint8_t>  {};
template <> struct primitiveSerializer<uint16_t> : simplePrimitiveSerializer<uint16_t> {};
template <> struct primitiveSerializer<uint32_t> : simplePrimitiveSerializer<uint32_t> {};
template <> struct primitiveSerializer<uint64_t> : simplePrimitiveSerializer<uint64_t> {};

template <> struct primitiveSerializer<int8_t>   : simplePrimitiveSerializer<int8_t>  {};
template <> struct primitiveSerializer<int16_t>  : simplePrimitiveSerializer<int16_t> {};
template <> struct primitiveSerializer<int32_t>  : simplePrimitiveSerializer<int32_t> {};
template <> struct primitiveSerializer<int64_t>  : simplePrimitiveSerializer<int64_t> {};

template <> struct primitiveSerializer<float>    : simplePrimitiveSerializer<float>  {};
template <> struct primitiveSerializer<double>   : simplePrimitiveSerializer<double> {};

template <typename T>
concept PrimitiveType = requires(T a) {
	(BinarySerializer<T>)primitiveSerializer<T>::serializeBytes;
	(BinaryDeserializer<T>)primitiveSerializer<T>::deserializeBytes;
	(BinarySerializedSize<T>)primitiveSerializer<T>::serializedByteSize;
};

template <typename T>
concept BinarySerializeableClass
	= requires(T a) {
		(BinarySerializer<T>)T::serializeBytes;
		(BinaryDeserializer<T>)T::deserializeBytes;
		(BinarySerializedSize<T>)T::serializedByteSize;
	};

template <typename T>
concept BinarySerializeableType = BinarySerializeableClass<T>
                               || PrimitiveType<T>;

template <typename T>
struct serializerImpl {
	BinarySerializer<T>     serialize;
	BinaryDeserializer<T>   deserialize;
	BinarySerializedSize<T> size;
};

template <BinarySerializeableClass T>
struct serializerImpl<T> getSerializerImpl() {
	return {
		.serialize   = T::serializeBytes,
		.deserialize = T::deserializeBytes,
		.size        = T::serializedByteSize,
	};
}

template <typename T>
struct serializerImpl<T> getSerializerImpl() {
	using P = primitiveSerializer<T>;

	return {
		.serialize   = P::serializeBytes,
		.deserialize = P::deserializeBytes,
		.size        = P::serializedByteSize,
	};
}

template <BinarySerializeableType T>
class bufferComponent : public component {
	public:
		bufferComponent(regArgs t)
			: component(doRegister(this, t)) {};

		std::vector<T> data;

		// TODO:
		static nlohmann::json serializer(component *comp) {
			LogInfo("Got here!");
			auto serializer = getSerializerImpl<T>();
			auto *self = static_cast<bufferComponent<T>*>(comp);
			size_t length = self->data.size() * serializer.size();
			uint8_t *buf = new uint8_t[length];

			for (size_t i = 0; i < self->data.size(); i++) {
				serializer.serialize(self->data.data() + i, buf, i*serializer.size());
			}

			// TODO: check to see if it's okay to pass char* directly in json return
			std::unique_ptr<char> data(base64_encode_binary(buf, length, 0));
			std::string temp(data.get());

			return {{"data", temp}};
		}

		static void deserializer(component *comp, nlohmann::json j) {
			LogInfo("Deserializer!");

			auto serializer = getSerializerImpl<T>();
			auto *self = static_cast<bufferComponent<T>*>(comp);
			auto& dataobj = j["data"];

			if (!dataobj.is_string()) {
				return;
			}

			const std::string& data = dataobj;

			uint8_t *buffer;
			size_t  length;
			base64_decode_binary(data.c_str(), &buffer, &length);

			size_t elems = length / serializer.size();
			self->data.resize(elems);

			for (size_t i = 0; i < elems; i++) {
				serializer.deserialize(self->data.data(), buffer, i*serializer.size());
			}

			free(buffer);
		};

		static void drawEditor(component *comp) {};
};

// namespace grendx::ecs
}
