#pragma once

#include <grend/gameObject.hpp>
#include <grend/ecs/ecs.hpp>
#include "area.hpp"

using namespace grendx;
using namespace grendx::ecs;

class flag : public entity {
	public:
		flag(entityManager *manager, gameMain *game,
		     glm::vec3 position, std::string color);

		virtual ~flag();
		virtual void update(entityManager *manager, float delta);
};

class hasFlag : public component {
	public:
		hasFlag(entityManager *manager, entity *ent, std::string _color)
			: component(manager, ent),
			  color(_color)
		{
			manager->registerComponent(ent, "hasFlag", this);
		}

		virtual ~hasFlag();
		std::string color;
};

class flagPickup : public areaEnter {
	public:
		flagPickup(entityManager *manager, entity *ent)
			: areaEnter(manager, ent, {"flag", "area"})
		{
			manager->registerComponent(ent, "flagPickup", this);
		}

		virtual ~flagPickup();
		virtual void onEvent(entityManager *manager, entity *ent, entity *other);
};
