#pragma once

#include <grend/ecs/ecs.hpp>
#include <grend/physics.hpp>

namespace grendx::ecs {

class collisionHandler : public component {
	public:
		collisionHandler(regArgs t,
		                 std::initializer_list<const char *> tags);

		virtual ~collisionHandler();

		virtual void
		onCollision(entityManager *manager, entity *ent,
		            entity *other, collision& col) = 0;

		std::vector<const char *> tags;

		// serialization stuff
		constexpr static const char *serializedType = "collisionHandler";

		virtual const char* typeString(void) const { return serializedType; };
		virtual nlohmann::json serialize(entityManager *manager);
};

class entitySystemCollision : public entitySystem {
	public:
		typedef std::shared_ptr<entitySystem> ptr;
		typedef std::weak_ptr<entitySystem>   weakptr;

		virtual ~entitySystemCollision();
		virtual void update(entityManager *manager, float delta);
};

// namespace grendx::ecs
}
