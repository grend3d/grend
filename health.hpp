#pragma once

#include <grend/ecs/ecs.hpp>
#include <grend/utility.hpp>

using namespace grendx;
using namespace grendx::ecs;

class health : public component {
	public:
		health(entityManager *manager, entity *ent,
		       float _amount = 1.f, float _hp = 100.f)
			: component(manager, ent),
			  amount(_amount), hp(_hp)
		{
			manager->registerComponent(ent, "health", this);
		}

		virtual float damage(float damage) {
			amount = max(0.0, amount - damage/hp);
			return amount*hp;
		}

		virtual float decrement(float dec) {
			amount = max(0.0, amount - dec);
			return amount;
		}

		float amount;
		float hp;
};
