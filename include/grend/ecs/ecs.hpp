#pragma once

#include <grend/gameObject.hpp>
#include <nlohmann/json.hpp>

#include <iostream>
#include <map>
#include <vector>
#include <set>
#include <string>
#include <memory>
#include <initializer_list>

// XXX: forward declaration for gameMain.hpp (at end)
//      entityManager keeps a gameMain pointer, avoids pointlessly having to
//      pass it as a function parameter all the time
namespace grendx { class gameMain; }

namespace grendx::ecs {

class component;
class entity;
class entitySystem;
class entityEventSystem;

class entityManager {
	public:
		typedef std::shared_ptr<entityManager> ptr;
		typedef std::weak_ptr<entityManager>   weakptr;

		entityManager(gameMain *_engine) : engine(_engine) {};
		~entityManager();

		std::map<std::string, std::shared_ptr<entitySystem>> systems;
		std::map<std::string, std::shared_ptr<entityEventSystem>> addEvents;
		std::map<std::string, std::shared_ptr<entityEventSystem>> removeEvents;
		// TODO: probably want externally-specified event systems

		std::map<std::string, std::set<component*>> components;
		std::map<entity*, std::multimap<std::string, component*>> entityComponents;
		std::map<component*, entity*> componentEntities;
		std::set<entity*> entities;
		std::set<entity*> added;
		std::set<entity*> condemned;

		void update(float delta);
		void registerComponent(entity *ent, std::string name, component *ptr);

		void add(entity *ent);
		void remove(entity *ent);
		void activate(entity *ent);
		void deactivate(entity *ent);

		bool hasComponents(entity *ent, std::initializer_list<std::string> tags);
		bool hasComponents(entity *ent, std::vector<std::string> tags);

		// remove() doesn't immediately free, just adds the entity
		// to a queue of the condemned, clearFreedEntities() banishes the
		// condemned from memory
		void freeEntity(entity *ent);
		void clearFreedEntities(void);

		std::set<component*>& getComponents(std::string name);
		std::multimap<std::string, component*>& getEntityComponents(entity *ent);
		entity *getEntity(component *com); 

		gameObject::ptr root = std::make_shared<gameObject>();

		// TODO: should this be similar to inputHandlerSystem, with
		//       onCollision components?
		std::shared_ptr<std::vector<collision>> collisions
			= std::make_shared<std::vector<collision>>();

		// XXX
		gameMain *engine;
};

class component {
	public:
		constexpr static const char *serializedType = "component";
		static const nlohmann::json defaultProperties(void) {
			return {};
		};

		component(entityManager *manager,
		          entity *ent,
		          nlohmann::json properties = defaultProperties())
		{
			manager->registerComponent(ent, serializedType, this);
		}

		virtual ~component();
		virtual const char* typeString(void) const { return serializedType; };
		virtual nlohmann::json serialize(entityManager *manager) { return {}; };

		// draw the imgui widgets for this component, TODO
		// also, this class needs a polymorphic member in order to upcast,
		// so this fills that role.
		virtual void drawEditorWidgets(void) { };
};

class entity : public component {
	public:
		constexpr static const char *serializedType = "entity";
		static const nlohmann::json defaultProperties(void) {
			return {
				{"type",        "entity"},
				{"entity-type", serializedType},
				{"node", {
					{"position", {0.f, 0.f, 0.f}},
					{"rotation", {1.f, 0.f, 0.f, 0.f}},
					{"scale",    {1.f, 1.f, 1.f}},
				}},
				{"components", {}},
			};
		};

		typedef std::shared_ptr<entity> ptr;
		typedef std::weak_ptr<entity>   weakptr;

		entity(entityManager *manager,
		       nlohmann::json properties = defaultProperties());

		entity(entityManager *manager,
		       entity *ent,
		       nlohmann::json properties);

		virtual ~entity();
		virtual const char* typeString(void) const { return "entity"; };
		virtual nlohmann::json serialize(entityManager *manager);

		virtual void update(entityManager *manager, float delta) {};
		virtual gameObject::ptr getNode(void) { return node; };

		gameObject::ptr node = std::make_shared<gameObject>();
		// TODO: should have a seperate entity list for deactivated
		//       entities, where being in that list is what decides whether
		//       an entity is deactivated or not
		bool active = true;
};

class entitySystem {
	public:
		typedef std::shared_ptr<entitySystem> ptr;
		typedef std::weak_ptr<entitySystem>   weakptr;

		virtual ~entitySystem();
		virtual void update(entityManager *manager, float delta) {}
};

class entityEventSystem {
	public:
		typedef std::shared_ptr<entityEventSystem> ptr;
		typedef std::weak_ptr<entityEventSystem>   weakptr;

		entityEventSystem(std::vector<std::string> _tags) : tags(_tags) {};

		virtual ~entityEventSystem();
		virtual void onEvent(entityManager *manager, entity *ent, float delta) {};

		std::vector<std::string> tags;
};

std::set<entity*> searchEntities(entityManager *manager,
                                 std::initializer_list<std::string> tags);
std::set<entity*> searchEntities(entityManager *manager,
                                 std::vector<std::string>& tags);

entity *findNearest(entityManager *manager,
                    glm::vec3 position,
                    std::initializer_list<std::string> tags);

entity *findFirst(entityManager *manager,
                  std::initializer_list<std::string> tags);

template <class T>
T castEntityComponent(entityManager *m, entity *e, std::string name) {
	auto &comps = m->getEntityComponents(e);
	auto comp = comps.find(name);

	if (comp == comps.end()) {
		// TODO: error?
		return nullptr;
	}

	return dynamic_cast<T>(comp->second);
}

template <class T>
T castEntityComponent(T& val, entityManager *m, entity *e, std::string name) {
	val = castEntityComponent<T>(m, e, name);
	return val;
}

// TODO: generalized for any iterable container type
bool intersects(std::multimap<std::string, component*>& entdata,
                std::initializer_list<std::string> test);

bool intersects(std::multimap<std::string, component*>& entdata,
                std::vector<std::string>& test);

// namespace grendx::ecs
};

#include <grend/gameMain.hpp>
