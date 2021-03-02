#pragma once

#include <grend/gameObject.hpp>
#include <grend/ecs/ecs.hpp>

#include <iostream>
#include <map>
#include <vector>
#include <set>
#include <string>
#include <memory>
#include <initializer_list>

#include <nlohmann/json.hpp>

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
		virtual component *allocate(entityManager *manager,
		                            entity *ent,
		                            nlohmann::json properties={}) = 0;
};

template <class T>
class factory : public abstractFactory {
	public:
		factory() {};
		virtual ~factory() {};

		virtual component *allocate(entityManager *manager,
		                            entity *ent,
		                            nlohmann::json properties={})
		{
			return new T(manager, ent, properties);
		}
};

class factories {
	public:
		typedef std::shared_ptr<factories> ptr;
		typedef std::weak_ptr<factories>   weakptr;

		void add(std::string name, abstractFactory::ptr ser) {
			factories[name] = ser;
		}

		template <class T>
		void add() {
			std::cerr << "Registering factory for component "
				<< T::serializedType
				<< std::endl;

			factories[T::serializedType] = std::make_shared<factory<T>>();
			defaults[T::serializedType]  = T::defaultProperties();
		}

		bool has(std::string name) {
			auto f = factories.find(name);
			auto d = defaults.find(name);

			return f != factories.end() && d != defaults.end();
		}

		nlohmann::json properties(std::string name) {
			auto it = defaults.find(name);

			if (it != defaults.end()) {
				return it->second;
			}

			return {};
		}

		entity    *build(entityManager *manager, nlohmann::json serialized);
		component *build(entityManager *manager,
		                 entity *ent,
		                 nlohmann::json serialized);

		std::map<std::string, abstractFactory::ptr> factories;
		std::map<std::string, nlohmann::json> defaults;
};

// namespace grendx::ecs
}
