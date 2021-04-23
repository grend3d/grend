#pragma once

#include <grend/gameObject.hpp>
#include <grend/ecs/ecs.hpp>
#include <components/area.hpp>

using namespace grendx;
using namespace grendx::ecs;

class targetArea : public entity {
	public:
		targetArea(entityManager *manager, entity *ent, nlohmann::json properties);
		virtual ~targetArea();

		// serialization stuff
		constexpr static const char *serializedType = "targetArea";
		static const nlohmann::json defaultProperties(void) {
			return entity::defaultProperties();
		}

		virtual const char *typeString(void) const { return serializedType; };
		virtual nlohmann::json serialize(entityManager *manager); 
};

class areaAddScore : public areaInside {
	public:
		areaAddScore(entityManager *manager, entity *ent, nlohmann::json properties);
		virtual ~areaAddScore();

		virtual void onEvent(entityManager *manager, entity *ent, entity *other);

		// serialization stuff
		constexpr static const char *serializedType = "areaAddScore";
		static const nlohmann::json defaultProperties(void) {
			return component::defaultProperties();
		}

		virtual const char *typeString(void) const { return serializedType; };
		virtual nlohmann::json serialize(entityManager *manager); 
};
