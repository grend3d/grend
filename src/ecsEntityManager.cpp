#include <grend/ecs/ecs.hpp>

namespace grendx::ecs {

void entityManager::update(float delta) {
	for (auto& [name, system] : systems) {
		if (system) {
			system->update(this, delta);
		}
	}

	for (auto& ent : entities) {
		ent->update(this, delta);
	}

	collisions->clear();
}

void entityManager::add(entity *ent) {
	setNode("entity["+std::to_string((uintptr_t)ent)+"]", root, ent->getNode());
	entities.insert(ent);
}

void entityManager::remove(entity *ent) {
	if (!entities.count(ent)) {
		// not a valid entity
		return;
	}

	root->removeNode("entity["+std::to_string((uintptr_t)ent)+"]");

	auto comps = entityComponents[ent];

	for (auto& [name, comp] : comps) {
		componentEntities.erase(comp);
		components[name].erase(comp);
	}

	entityComponents.erase(ent);
	entities.erase(ent);
	//delete ent; // TODO: don't leak memory ok
}

// TODO: specialization or w/e
bool entityManager::hasComponents(entity *ent,
                                  std::initializer_list<std::string> tags)
{
	auto& compmap = entityComponents[ent];
	return intersects(compmap, tags);
}

bool entityManager::hasComponents(entity *ent,
                                  std::vector<std::string> tags)
{
	auto& compmap = entityComponents[ent];
	return intersects(compmap, tags);
}

std::set<entity*> searchEntities(entityManager *manager,
                                 std::initializer_list<std::string> tags)
{
	std::set<entity*> ret;
	std::vector<std::set<entity*>> candidates;

	/*
	// left here for debugging, just in case
	for (auto& ent : manager->entities) {
		if (manager->hasComponents(ent, tags)) {
			ret.insert(ent);
		}
	}
	*/

	if (tags.size() == 0) {
		// lol
		return {};

	} else if (tags.size() == 1) {
		auto& comps = manager->getComponents(*tags.begin());

		for (auto& comp : comps) {
			ret.insert(manager->getEntity(comp));
		}

	} else {
		// TODO: could this be done without generating the sets of entities...?
		//       would need a map of component names -> entities, which would be
		//       messy since an entity can have more than one of any component...
		//       this is probably good enough, just don't search for "entity" :P
		for (auto& tag : tags) {
			auto& comps = manager->getComponents(tag);
			candidates.push_back({});

			for (auto& comp : comps) {
				candidates.back().insert(manager->getEntity(comp));
			}
		}

		// find smallest (most exclusive) set of candidates
		auto smallest = candidates.begin();
		for (auto it = candidates.begin(); it != candidates.end(); it++) {
			if (it->size() < smallest->size()) {
				smallest = it;
			}
		}

		for (auto ent : *smallest) {
			bool inAll = true;

			for (auto it = candidates.begin(); it != candidates.end(); it++) {
				if (it == smallest)  continue;

				if (!it->count(ent)) {
					inAll = false;
					break;
				}
			}

			if (inAll) {
				ret.insert(ent);
			}
		}
	}

	return ret;
}

entity *findNearest(entityManager *manager,
                    glm::vec3 position,
                    std::initializer_list<std::string> tags)
{
	float curmin = HUGE_VALF;
	entity *ret = nullptr;

	auto ents = searchEntities(manager, tags);
	for (auto& ent : ents) {
		gameObject::ptr node = ent->getNode();
		float dist = glm::distance(position, node->transform.position);

		if (dist < curmin) {
			curmin = dist;
			ret = ent;
		}
	}

	return ret;
}

entity *findFirst(entityManager *manager,
                  std::initializer_list<std::string> tags)
{
	// TODO: search by component names
	for (auto& ent : manager->entities) {
		if (manager->hasComponents(ent, tags)) {
			return ent;
		}
	}

	return nullptr;
}


std::set<component*>& entityManager::getComponents(std::string name) {
	// XXX: avoid creating component names if nothing registered them
	static std::set<component*> nullret;
	//std::cerr << "looking up components for " << name << std::endl;

	return (components.count(name))? components[name] : nullret;
}

std::multimap<std::string, component*>&
entityManager::getEntityComponents(entity *ent) {
	// XXX: similarly, something which isn't registered here has 
	//      no components (at least, in this system) so return empty set
	static std::multimap<std::string, component*> nullret;

	return (entityComponents.count(ent))? entityComponents[ent] : nullret;
}

entity *entityManager::getEntity(component *com) {
	auto it = componentEntities.find(com);
	if (it != componentEntities.end()) {
		return it->second;

	} else {
		return nullptr;
	}
}

void entityManager::registerComponent(entity *ent,
                                      std::string name,
                                      component *ptr)
{
	// TODO: need a proper logger
	std::cerr << "registering component '" << name << "' for " << ptr << std::endl;
	components[name].insert(ptr);
	entityComponents[ent].insert({name, ptr});
	componentEntities.insert({ptr, ent});
}

bool intersects(std::multimap<std::string, component*>& entdata,
                std::initializer_list<std::string> test)
{
	for (auto& s : test) {
		auto it = entdata.find(s);
		if (it == entdata.end()) {
			return false;
		}
	}

	return true;
}

bool intersects(std::multimap<std::string, component*>& entdata,
                std::vector<std::string>& test)
{
	for (auto& s : test) {
		auto it = entdata.find(s);
		if (it == entdata.end()) {
			return false;
		}
	}

	return true;
}

// namespace grendx::ecs
};
