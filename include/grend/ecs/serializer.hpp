#pragma once

#include <grend/sceneNode.hpp>
#include <grend/ecs/ecs.hpp>

#include <iostream>
#include <map>
#include <vector>
#include <set>
#include <string>
#include <memory>
#include <initializer_list>

#include <nlohmann/json.hpp>
#include <grend/utility.hpp>

// TODO: add 'serialize' namespace?
namespace grendx::ecs {

// abstract component bean serializer factory
// OOP to the max
//
// (ftr I'm not a big fan of this, but it seems like the least crap way to
//  deal with (de)serialization in the ECS I've set up here, which I do like,
//  so eh...)
class abstractFactory {
	public:
		typedef std::shared_ptr<abstractFactory> ptr;
		typedef std::weak_ptr<abstractFactory>   weakptr;

		virtual ~abstractFactory();
		virtual component *allocate(entityManager *manager, entity *ent) = 0;
};

template <class T>
class factory : public abstractFactory {
	public:
		factory() {};
		virtual ~factory() {};

		virtual component *allocate(entityManager *manager, entity *ent)
		{
			return new T(manager, ent);
		}
};

using SerializeFunc   = std::function<nlohmann::json(component *comp)>;
using DeserializeFunc = std::function<bool(component *comp, nlohmann::json& properties)>;

template <typename T>
concept InlineSerializers
	= requires(T a) {
		static_cast<SerializeFunc>(T::serialize);
		static_cast<DeserializeFunc>(T::deserialize);
		//SerializeFunc   ser = T::serialize;
		//DeserializeFunc des = T::deserialize;
	};

class serializer {
	public:
		typedef std::shared_ptr<serializer> ptr;
		typedef std::weak_ptr<serializer>   weakptr;

		template <class T>
		void add(const SerializeFunc& ser, const DeserializeFunc& deser) {
			const char *name = getTypeName<T>();
			const std::string& demangled = demangle(name);

			std::cerr << "Registering factory for component "
				<< demangled << " (" << name << "@" << (void*)name << ")"
				<< std::endl;

			factories[demangled]     = std::make_shared<factory<T>>();
			serializers[demangled]   = ser;
			deserializers[demangled] = deser;
		}

		template <InlineSerializers T>
		void add() {
			add<T>(T::serializer, T::deserializer);
		}

		bool has(const std::string& name) {
			auto f = factories.find(name);
			// shouldn't need to check for serializers/deserializers,
			// by construction there should never be a factory without corresponding
			// serialization functions
			return f != factories.end();
		}

		entity    *build(entityManager *manager, nlohmann::json serialized);
		component *build(entityManager *manager,
		                 entity *ent,
		                 nlohmann::json serialized);

		std::map<std::string, abstractFactory::ptr> factories;
		std::map<std::string, SerializeFunc>   serializers;
		std::map<std::string, DeserializeFunc> deserializers;
};

// namespace grendx::ecs
}
