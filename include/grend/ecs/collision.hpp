#pragma once

#include <grend/ecs/ecs.hpp>

namespace grendx::ecs {

class collisionHandler : public component {
	public:
		// XXX: hmm, need some way to avoid ambiguous constructors when
		//      initializing with std::vector<std::string> tag list
		collisionHandler(entityManager *manager,
		                 entity *ent,
		                 nlohmann::json properties);

		virtual ~collisionHandler();

		virtual void
		onCollision(entityManager *manager, entity *ent,
		            entity *other, collision& col) = 0;

		std::vector<const char *> tags;
		// XXX: storage for tags
		std::vector<std::string> tagstore;

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
