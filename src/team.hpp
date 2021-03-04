#pragma once

#include <grend/gameObject.hpp>
#include <grend/ecs/ecs.hpp>

using namespace grendx;
using namespace grendx::ecs;

class team : public component {
	public:
		team(entityManager *manager, entity *ent, std::string teamname)
			: component(manager, ent),
		      name(teamname)
		{
			manager->registerComponent(ent, "team", this);
		};

		virtual ~team();
		virtual bool operator==(const team& other) const;

		std::string name;

		// serialization stuff
		constexpr static const char *serializedType = "team";
		static const nlohmann::json defaultProperties(void) {
			return component::defaultProperties();
		}

		virtual const char *typeString(void) const { return serializedType; };
		virtual nlohmann::json serialize(entityManager *manager);
};
