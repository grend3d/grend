#pragma once

#include <grend/ecs/ecs.hpp>

namespace grendx::ecs {

class collisionHandler : public component {
	public:
		collisionHandler(entityManager *manager, entity *ent,
		                 std::vector<std::string> _tags)
			: component(manager, ent),
			  tags(_tags)
		{
			manager->registerComponent(ent, "collisionHandler", this);
		}

		virtual ~collisionHandler() {};

		virtual void
		onCollision(entityManager *manager, entity *ent,
		            entity *other, collision& col) = 0;

		std::vector<std::string> tags;
};

class entitySystemCollision : public entitySystem {
	public:
		typedef std::shared_ptr<entitySystem> ptr;
		typedef std::weak_ptr<entitySystem>   weakptr;

		virtual void update(entityManager *manager, float delta);
};

// namespace grendx::ecs
}
