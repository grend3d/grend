#include <grend/ecs/ecs.hpp>
#include <grend/ecs/search.hpp>

namespace grendx::ecs {

// key functions for rtti
component::~component() {
	std::cerr << "got to ~component() "
		<< "[type: " << this->typeString() << "]"
		<< std::endl;
}

entity::~entity() {};
entitySystem::~entitySystem() {};
entityEventSystem::~entityEventSystem() {};


entity::entity(entityManager *manager,
		       nlohmann::json properties)
	: component(manager, this, properties)
{
	manager->registerComponent(this, "entity", this);

	glm::vec3 pos(properties["node"]["position"][0],
	              properties["node"]["position"][1],
	              properties["node"]["position"][2]);

	glm::quat rot(properties["node"]["rotation"][0],
	              properties["node"]["rotation"][1],
	              properties["node"]["rotation"][2],
	              properties["node"]["rotation"][3]);

	glm::vec3 scale(properties["node"]["scale"][0],
	                properties["node"]["scale"][1],
	                properties["node"]["scale"][2]);

	node->setTransform((TRS) {
		.position = pos,
		.rotation = rot,
		.scale    = scale,
	});
}

// TODO: register entities as components of other entities...
entity::entity(entityManager *manager,
               entity *ent,
               nlohmann::json properties)
	: entity(manager, properties)
{
	
}

void entityManager::update(float delta) {
	for (auto& [name, system] : systems) {
		if (system) {
			system->update(this, delta);
		}
	}

	for (auto& ent : entities) {
		ent->update(this, delta);
	}

	for (auto& ent : added) {
		for (auto& [_, sys] : addEvents) {
			// TODO: not very efficient, need some sort of system indexing
			//       to reduce overhead when there's lots of systems
			if (hasComponents(ent, sys->tags)) {
				sys->onEvent(this, ent, delta);
			}
		}
	}

	for (auto& ent : condemned) {
		for (auto& [_, sys] : removeEvents) {
			// TODO: not very efficient, need some sort of system indexing
			//       to reduce overhead when there's lots of systems
			if (hasComponents(ent, sys->tags)) {
				sys->onEvent(this, ent, delta);
			}
		}
	}

	collisions->clear();
	added.clear();
	clearFreedEntities();
}

void entityManager::add(entity *ent) {
	SDL_Log("Adding entity %s", ent->typeString());
	setNode("entity["+std::to_string((uintptr_t)ent)+"]", root, ent->getNode());
	entities.insert(ent);
	added.insert(ent);
}

void entityManager::remove(entity *ent) {
	condemned.insert(ent);
}

entityManager::~entityManager() {
	for (auto& it : entities) {
		remove(it);
	}

	clearFreedEntities();
}

void entityManager::clearFreedEntities(void) {
	for (auto& ent : condemned) {
		freeEntity(ent);
	}

	condemned.clear();
}

void entityManager::freeEntity(entity *ent) {
	if (!entities.count(ent)) {
		// not a valid entity
		return;
	}

	root->removeNode("entity["+std::to_string((uintptr_t)ent)+"]");

	// first free component objects
	auto comps = entityComponents[ent];
	auto uniqueComponents = comps.equal_range("component");

	for (auto it = uniqueComponents.first; it != uniqueComponents.second; it++) {
		auto& [name, comp] = *it;
		auto subent = entities.find((entity*)comp);

		if (subent != entities.end()) {
			entity *sub = dynamic_cast<entity*>(comp);

			// entities get a self-referential entity and base component,
			// need to check for that
			if (sub && sub != ent) {
				SDL_Log("entity has subentity, deleting that too");
				remove(sub);
			}
		}

		// also, since entities have a self-referential component this deletes
		// the entity object as well
		delete comp;
	}

	// then remove pointers from indexes
	for (auto& [name, comp] : comps) {
		SDL_Log("removing component %s from entity", name.c_str());
		componentEntities.erase(comp);
		components[name].erase(comp);
	}

	entityComponents.erase(ent);
	entities.erase(ent);
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
	return searchEntities<std::initializer_list<std::string>>
		(manager, {tags.begin(), tags.end()}, tags.size());
}

std::set<entity*> searchEntities(entityManager *manager,
                                 std::vector<std::string>& tags)
{
	//return searchEntities(manager, {tags.data(), tags.data() + tags.size()});
	return searchEntities<std::vector<std::string>>
		(manager, {tags.begin(), tags.end()}, tags.size());
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
		float dist = glm::distance(position, node->getTransformTRS().position);

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
	SDL_Log("registering component '%s' for %p", name.c_str(), ptr);
	components[name].insert(ptr);
	entityComponents[ent].insert({name, ptr});
	componentEntities.insert({ptr, ent});
}

nlohmann::json entity::serialize(entityManager *manager) {
	nlohmann::json components = {};

	auto comps = manager->getEntityComponents(this);
	auto uniq  = comps.equal_range("component");

	for (auto it = uniq.first; it != uniq.second; it++) {
		auto& [_, comp] = *it;

		// entities have themselves as components, don't recurse infinitely
		if (comp != this) {
			components.push_back({ comp->typeString(), comp->serialize(manager) });
		}
	}

	TRS transform = node->getTransformTRS();

	return {
		{"type",        "entity"},
		{"entity-type", this->typeString()},

		{"node", {
			{"position",
				{
					transform.position.x,
					transform.position.y,
					transform.position.z,
				}},
			{"rotation",
				{
					transform.rotation.w,
					transform.rotation.x,
					transform.rotation.y,
					transform.rotation.z,
				}},
			{"scale",
				{
					transform.scale.x,
					transform.scale.y,
					transform.scale.z,
				}},
		}},

		{"components",  components}, 
	};
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
