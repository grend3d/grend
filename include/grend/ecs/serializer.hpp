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

class componentSerializer {
	public:
		typedef std::shared_ptr<componentSerializer> ptr;
		typedef std::weak_ptr<componentSerializer>   weakptr;

		// this is to make linking the serializer to the registry less of a pain,
		// so don't need to worry about accessing this from derived classes
		// TODO: is there a way to avoid having this be visible from derived classes,
		//       but still accessible externally?
		//constexpr static const char *serializedType = "component";

		componentSerializer() {};
		virtual ~componentSerializer() {};

		virtual component *allocate(entityManager *manager,
		                            entity *ent,
		                            nlohmann::json source)
		{
			return new component(manager, ent);
		}

		virtual nlohmann::json serialize(component *comp) {
			return {};
		}

		virtual void deserialize(component *comp, nlohmann::json source) {
			// deserialize stuff here
		}
};

class serializerRegistry {
	public:
		typedef std::shared_ptr<serializerRegistry> ptr;
		typedef std::weak_ptr<serializerRegistry>   weakptr;

		void add(std::string name, componentSerializer::ptr ser) {
			serializers[name] = ser;
		}

		template <class T>
		void add() {
			/*
			static_assert(T::serializedType != "component",
			              "you need to specify serializedType in your derived serializer class!");
						  */
			std::cerr << "Registered serializer " << T::serializedType << std::endl;
			componentSerializer::ptr ser = std::make_shared<T>();
			serializers[T::serializedType] = ser;
		}

		nlohmann::json serializeEntity(entityManager *manager, entity *ent);
		nlohmann::json serializeEntities(entityManager *manager);
		void deserializeEntity(entityManager *manager, entity *ent);
		std::map<std::string, componentSerializer::ptr> serializers;
};

// namespace grendx::ecs
}
