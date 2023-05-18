#pragma once

#include <grend/sceneNode.hpp>
#include <grend/ecs/ecs.hpp>
#include <grend/IoC.hpp>

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

// SFINAE hax, dispatch to different constructors depending on whether
// a type is derived from the entity class
template <typename T>
consteval bool isEntityType() {
	return std::is_base_of<entity, T>::value;
}

template <typename T>
consteval bool isComponentType() {
	return  std::is_base_of<component, T>::value
	    && !std::is_base_of<entity, T>::value;
}

template <typename T>
static inline component* doConstruct(entityManager* manager, entity* ent,
                                     char(*)[isComponentType<T>()] = 0)
{
	return manager->construct<T>(ent);
}

template <typename T>
static inline component* doConstruct(entityManager* manager, entity* ent,
                                     char(*)[isEntityType<T>()] = 0)
{
	return manager->construct<T>();
}

template <class T>
class factory : public abstractFactory {
	public:
		factory() {};
		virtual ~factory() {};

		virtual component *allocate(entityManager *manager, entity *ent)
		{
			return doConstruct<T>(manager, ent);
		}
};

using SerializeFunc   = std::function<nlohmann::json(component *comp)>;
using DeserializeFunc = std::function<void(component *comp, nlohmann::json& properties)>;

template <typename T>
concept InlineSerializers
	= requires(T a) {
		(SerializeFunc)T::serializer;
		(DeserializeFunc)T::deserializer;
	};

template <typename T>
static inline
T tryGet(nlohmann::json& value, const std::string& name, T defaultVal) {
	return value.contains(name)? value[name].get<T>() : defaultVal;
}

class serializer : public IoC::Service {
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

		nlohmann::json serialize(entityManager *manager, entity *ent);

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
