#include <grend/ecs/ecs.hpp>
#include <grend/ecs/collision.hpp>

namespace grendx::ecs {

// key functions for rtti
collisionHandler::~collisionHandler() {};
entitySystemCollision::~entitySystemCollision() {};

/*
collisionHandler::collisionHandler(entityManager *manager, entity *ent,
                                   std::vector<std::string> _tags)
	: component(manager, ent),
	  tags(_tags)
{
	manager->registerComponent(ent, "collisionHandler", this);
}
*/

collisionHandler::collisionHandler(entityManager *manager,
                                   entity *ent,
                                   nlohmann::json properties)
	: component(manager, ent)
{
	manager->registerComponent(ent, "collisionHandler", this);

	for (auto& it : properties) {
		if (it.is_string()) {
			tags.push_back(it.get<std::string>());
		}
	}
}

nlohmann::json collisionHandler::serialize(entityManager *manager) {
	nlohmann::json tagsjson = {};

	for (auto& tag : tags) {
		tagsjson.push_back(tag);
	}

	return {{"tags", tagsjson}};
}

void entitySystemCollision::update(entityManager *manager, float delta) {
	for (auto& col : *manager->collisions) {
		if (!col.adata && !col.bdata) {
			// two collision events outside the ECS, nothing to do
			continue;
		}

		entity *ents[2] = {
			// TODO: maaaaaaybe dynamic cast... if we really don't care about
			//       performance :P
			//       (but would make other coexisting object systems possible...
			//        particularly a simpler lua scripting system, ECS is great
			//        but it does actually kinda suck for rapid prototyping (jams))
			static_cast<entity*>(col.adata),
			static_cast<entity*>(col.bdata),
		};

		for (unsigned i = 0; i < 2; i++) {
			entity *self  = ents[i];
			entity *other = ents[!i];

			if (!self) {
				// allow collisions with things outside the ECS
				// TODO: documentation noting that 'other' may be null!
				continue;
			}

			if (manager->hasComponents(self, {"collisionHandler", "entity"})) {
				auto entcomps = manager->getEntityComponents(self);
				auto range = entcomps.equal_range("collisionHandler");

				for (auto it = range.first; it != range.second; it++) {
					auto& [_, comp] = *it;

					collisionHandler *handler = dynamic_cast<collisionHandler*>(comp);
					if (!handler) {
						// no collision component...?
						continue;
					}

					if (handler->tags.empty()
					    || manager->hasComponents(other, handler->tags))
					{
						handler->onCollision(manager, self, other, col);
					}
				}
			}
		}
	}
}

// namespace grendx::ecs
}
