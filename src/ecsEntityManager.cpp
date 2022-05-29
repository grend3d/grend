#include <grend/ecs/ecs.hpp>
#include <grend/ecs/search.hpp>
#include <grend/ecs/sceneComponent.hpp>

namespace grendx::ecs {

// TODO: sceneComponent.cpp
// key function for rtti
sceneComponent::~sceneComponent() {};

// key functions for rtti
component::~component() {}

entity::~entity() {};
entitySystem::~entitySystem() {};
entityEventSystem::~entityEventSystem() {};
updatable::~updatable() {};
activatable::~activatable() {};

entity::entity(regArgs t)
	: component(doRegister(this, t))
{
	//manager->registerComponent(this, this);

	/*
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
	*/
}

/*
// TODO: register entities as components of other entities...
// TODO: keep this? I'm leaning more towards the idea of entity relationships
//       being tracked as components using an indexer system,
//       entity trees seem unwieldy and not as flexible
entity::entity(entityManager *manager, entity *ent, regArgs t)
	: component(manager, this, manager->registerComponent(this, t))
	//: entity(manager, )
{
}
*/

void entityManager::update(float delta) {
	for (auto& [name, system] : systems) {
		if (system) {
			// TODO: should also consider having an 'active' flag in systems
			//       so they can be toggled on and off as needed
			system->update(this, delta);
		}
	}

	//auto& updaters = getComponents<updatable>();
	auto& updaters = getComponents<updatable>();
	for (auto& comp : updaters) {
		entity *e = getEntity(comp);

		if (!e || !e->active) {
			continue;
		}

		if (updatable *u = dynamic_cast<updatable*>(comp)) {
			u->update(this, delta);

		} else {
			SDL_Log("(%s) Invalid updater!", e->typeString());
		}
	}

	/*
	for (auto& ent : entities) {
		if (ent->active) {
			ent->update(this, delta);
		}
	}
	*/

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
	if (!ent) {
		// TODO: warning
		return;
	}

	// TODO: toggleable messages
	//SDL_Log("Adding entity %s", ent->typeString());
	//setNode("entity["+std::to_string((uintptr_t)ent)+"]", root, ent->getNode());
	entities.insert(ent);
	added.insert(ent);
}

void entityManager::remove(entity *ent) {
	condemned.insert(ent);
}

bool entityManager::valid(entity *ent) {
	return entities.count(ent);
}

void entityManager::activate(entity *ent) {
	if (!valid(ent)) return;

	ent->active = ent->node->visible = true;

	// run activators, if any
	auto activators = ent->getAll<activatable>();

	for (auto it = activators.first; it != activators.second; it++) {
		activatable* act = dynamic_cast<activatable*>(it->second);
		if (act) {
			act->activate(this, ent);
		} else {
			SDL_Log("(%s) Invalid activator!", ent->typeString());
		}
	}
}

void entityManager::deactivate(entity *ent) {
	if (!valid(ent)) return;

	ent->active = ent->node->visible = false;

	// run deactivators, if any
	auto activators = ent->getAll<activatable>();

	for (auto it = activators.first; it != activators.second; it++) {
		activatable* act = dynamic_cast<activatable*>(it->second);
		if (act) {
			act->deactivate(this, ent);

		} else {
			SDL_Log("(%s) Invalid activator", ent->typeString());
		}
	}
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
	if (!valid(ent)) {
		// not a valid entity
		return;
	}

	//root->removeNode("entity["+std::to_string((uintptr_t)ent)+"]");

	// first free component objects
	auto comps = entityComponents[ent];
	auto uniqueComponents = ent->getAll<component>();

	for (auto it = uniqueComponents.first; it != uniqueComponents.second; it++) {
		auto& [name, comp] = *it;
		auto subent = entities.find((entity*)comp);

		if (subent != entities.end()) {
			entity *sub = static_cast<entity*>(comp);

			// entities get a self-referential entity and base component,
			// need to check for that
			if (sub != ent) {
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
		componentEntities.erase(comp);
		components[name].erase(comp);
	}

	entityComponents.erase(ent);
	entities.erase(ent);
}

// TODO: specialization or w/e
bool entityManager::hasComponents(entity *ent,
                                  std::initializer_list<const char *> tags)
{
	if (!valid(ent)) {
		return false;
	}

	auto& compmap = entityComponents[ent];
	return intersects(compmap, tags);
}

bool entityManager::hasComponents(entity *ent,
                                  std::vector<const char *> tags)
{
	if (!valid(ent)) {
		return false;
	}

	auto& compmap = entityComponents[ent];
	return intersects(compmap, tags);
}

std::set<entity*> searchEntities(entityManager *manager,
                                 std::initializer_list<const char *> tags)
{
	return searchEntities<std::initializer_list<const char *>>
		(manager, {tags.begin(), tags.end()}, tags.size());
}

std::set<entity*> searchEntities(entityManager *manager,
                                 std::vector<const char *>& tags)
{
	//return searchEntities(manager, {tags.data(), tags.data() + tags.size()});
	return searchEntities<std::vector<const char *>>
		(manager, {tags.begin(), tags.end()}, tags.size());
}

entity *findNearest(entityManager *manager,
                    glm::vec3 position,
                    std::initializer_list<const char *> tags)
{
	float curmin = HUGE_VALF;
	entity *ret = nullptr;

	auto ents = searchEntities(manager, tags);
	for (auto& ent : ents) {
		sceneNode::ptr node = ent->getNode();
		float dist = glm::distance(position, node->getTransformTRS().position);

		if (ent->active && dist < curmin) {
			curmin = dist;
			ret = ent;
		}
	}

	return ret;
}

entity *findFirst(entityManager *manager,
                  std::initializer_list<const char *> tags)
{
	for (auto& ent : manager->entities) {
		if (ent->active && manager->hasComponents(ent, tags)) {
			return ent;
		}
	}

	return nullptr;
}


/*
std::set<component*>& entityManager::getComponents(std::string name) {
	// XXX: avoid creating component names if nothing registered them
	static std::set<component*> nullret;

	return (components.count(name))? components[name] : nullret;
}
*/

std::multimap<const char *, component*>&
entityManager::getEntityComponents(entity *ent) {
	// XXX: similarly, something which isn't registered here has 
	//      no components (at least, in this system) so return empty set
	static std::multimap<const char *, component*> nullret;

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

regArgs entityManager::registerComponent(const char *name,
                                         component *ptr,
                                         const regArgs& t)
{
	// TODO: toggleable messages
	//SDL_Log("registering component '%s' for %p", demangle(name).c_str(), ptr);

	entity *ent = t.ent;

	components[name].insert(ptr);
	componentEntities.insert({ptr, ent});
	componentTypes[ptr].insert(name);
	entityComponents[ent].insert({name, ptr});

	return regArgs(t.manager, t.ent, {regArgs::you_should_not_construct_this_directly::magic::OK});
	//return t;
}

void entityManager::registerInterface(entity *ent,
                                      const char *name,
                                      //std::string name,
                                      void *ptr)
{
	// TODO: need a proper logger

	component *root;

#if 0 && GREND_BUILD_DEBUG
	if ((root = dynamic_cast<component*>(ptr)) == nullptr) {
		SDL_Log("Invalid interface, should be a base of a derived component!");
		SDL_Log("(as in, define a component that inherits from the interface)");
		return;
	}
#else
	root = static_cast<component*>(ptr);
#endif

	(void)registerComponent(name, root,
		// XXX: not this
	regArgs(nullptr, ent, {regArgs::you_should_not_construct_this_directly::magic::OK})
	//return t;
			);
}


// TODO: docs, noting possible linear time
void entityManager::unregisterComponent(entity *ent, component *ptr) {
	if (!valid(ent)) {
		return;
	}

	auto it = componentTypes.find(ptr);
	if (it == componentTypes.end())
		// doesn't exist in the entity manager, maybe double free?
		// TODO: debug-only log statement
		return;

	for (auto& name : it->second) {
		auto& comps = entityComponents[ent];
		auto  names = comps.equal_range(name);

		for (auto it = names.first; it != names.second;) {
			auto& [_, comp] = *it;

			it = (comp == ptr)? comps.erase(it) : std::next(it);
		}

		components[name].erase(ptr);
	}

	componentEntities.erase(ptr);
	componentTypes.erase(ptr);

	delete ptr;
}

void entityManager::unregisterComponentType(entity *ent, std::string name) {
	if (!valid(ent)) {
		return;
	}

	// TODO:
}

/*
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
*/

bool intersects(std::multimap<const char *, component*>& entdata,
                std::initializer_list<const char *> test)
{
	for (auto& s : test) {
		auto it = entdata.find(s);
		if (it == entdata.end()) {
			return false;
		}
	}

	return true;
}

bool intersects(std::multimap<const char *, component*>& entdata,
                std::vector<const char *>& test)
{
	for (auto& s : test) {
		auto it = entdata.find(s);
		if (it == entdata.end()) {
			return false;
		}
	}

	return true;
}

nlohmann::json entity::serializer(component *comp) {
	entity *ent = static_cast<entity*>(comp);
	const TRS& trs = ent->node->getTransformTRS();

	auto& t = trs.position;
	auto& r = trs.rotation;
	auto& s = trs.scale;

	return {
		{"position", {t.x, t.y, t.z}},
		{"rotation", {r.w, r.x, r.y, r.z}},
		{"scale",    {s.x, s.y, s.z}},
	};
}

void entity::deserializer(component *comp, nlohmann::json j) {
	entity *ent = static_cast<entity*>(comp);

	std::cout << "Got here! setting transform" << std::endl;

	auto& t = j["position"];
	auto& r = j["rotation"];
	auto& s = j["scale"];

	ent->node->setTransform((TRS) {
		.position = {t[0], t[1], t[2]},
		.rotation = {r[0], r[1], r[2], r[3]},
		.scale    = {s[0], s[1], s[2]},
	});
}

// namespace grendx::ecs
};
