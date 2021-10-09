/// @file
// TODO: more doxygen, copyright, etc
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
class entityManager;
class entitySystem;
class entityEventSystem;

template <typename T>
concept Nameable = requires(T a) {
	T::serializedType != nullptr;
	a.typeString();
};

template <typename T>
concept Constructable = requires(T a) {
	T::defaultProperties();
	new T((entityManager*)nullptr, (entity*)nullptr, T::defaultProperties());
};


template <typename T>
concept Serializeable
	=  Nameable<T>
	&& Constructable<T>
	&& requires(T a) { a.serialize(); };

class entityManager {
	public:
		typedef std::shared_ptr<entityManager> ptr;
		typedef std::weak_ptr<entityManager>   weakptr;

		entityManager(gameMain *_engine) : engine(_engine) {};
		~entityManager();

		// TODO: might be a good idea for state to be private
		std::map<std::string, std::shared_ptr<entitySystem>> systems;
		std::map<std::string, std::shared_ptr<entityEventSystem>> addEvents;
		std::map<std::string, std::shared_ptr<entityEventSystem>> removeEvents;
		// TODO: probably want externally-specified event systems

		std::map<std::string, std::set<component*>> components;
		std::map<entity*, std::multimap<std::string, component*>> entityComponents;
		std::map<component*, entity*> componentEntities;
		std::map<component*, std::set<std::string>> componentTypes;
		std::set<entity*> entities;
		std::set<entity*> added;
		std::set<entity*> condemned;

		void registerComponent(entity *ent, std::string name, component *ptr);
		template <Nameable T>
		void registerComponent(entity *ent, T *ptr) {
			registerComponent(ent, T::serializedType, ptr);
		};

		void unregisterComponent(entity *ent, component *ptr);
		void unregisterComponentType(entity *ent, std::string name);
		// TODO: will there ever be a time where I'd only want to unregister
		//       a component from one specific type?

		void add(entity *ent);
		void remove(entity *ent);
		bool valid(entity *ent);

		void update(float delta);
		void activate(entity *ent);
		void deactivate(entity *ent);

		bool hasComponents(entity *ent, std::initializer_list<std::string> tags);
		bool hasComponents(entity *ent, std::vector<std::string> tags);

		template <Nameable T>
		bool hasComponent(entity *ent) {
			return hasComponents(ent, {T::serializedType});
		};

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

		virtual gameObject::ptr getNode(void) { return node; };

		gameObject::ptr node = std::make_shared<gameObject>();
		// TODO: should have a seperate entity list for deactivated
		//       entities, where being in that list is what decides whether
		//       an entity is deactivated or not
		bool active = true;
};

/**
 * Interface for components with per-frame updates.
 *
 * Entities and components should inherit from this base class,
 * implement update(), and register themselves as an "updatable"
 * component on their containing entity. (For entities that means
 * themselves.)
 */
class updatable {
	public:
		// TODO: probably a good idea to have a constructor
		//       here that automatically registers this thing
		//       as an updatable component
		virtual ~updatable();

		//! Update state, integrated over the time difference from
		//! the last frame.
		virtual void update(entityManager *manager, float delta) = 0;
};

/**
 * Interface for actions to perform when activating/deactivating
 * entities.
 *
 * To use this class, entities should inherit from this base class,
 * implement activate() and deactivate(), and register themselves as
 * an "activatable" component on themselves. 
 */
class activatable {
	public:
		virtual ~activatable();

		virtual void activate(entityManager *manager, entity *ent) = 0;
		virtual void deactivate(entityManager *manager, entity *ent) = 0;
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

template <Nameable T>
T* findNearest(entityManager *manager, glm::vec3 position) {
	return (T*)findNearest(manager, position, {T::serializedType});
}

template <Nameable T>
T* findFirst(entityManager *manager) {
	return (T*)findFirst(manager, {T::serializedType});
}

template <class T>
T castEntityComponent(entityManager *m, entity *e, std::string name) {
	auto &comps = m->getEntityComponents(e);
	auto comp = comps.find(name);

	if (comp == comps.end()) {
		// TODO: error?
		return nullptr;
	}

	// TODO: dynamic_cast for debug, static_cast for production?
	//       seems like a recipe for bugs that disappear in debug mode
	//return dynamic_cast<T>(comp->second);
	return static_cast<T>(comp->second);
}

template <class T>
T castEntityComponent(T& val, entityManager *m, entity *e, std::string name) {
	val = castEntityComponent<T>(m, e, name);
	return val;
}

template <Nameable T>
T* castEntityComponent(entityManager *m, entity *e) {
	return castEntityComponent<T*>(m, e, T::serializedType);
}

// TODO: generalized for any iterable container type
bool intersects(std::multimap<std::string, component*>& entdata,
                std::initializer_list<std::string> test);

bool intersects(std::multimap<std::string, component*>& entdata,
                std::vector<std::string>& test);

// namespace grendx::ecs
};

#include <grend/gameMain.hpp>
