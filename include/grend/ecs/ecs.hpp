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
const char *getTypeName() {
	return typeid(T).name();
}

template <typename T>
const char *getTypeName(T& thing) {
	return typeid(T).name();
}

/*
template <typename T, typename U>
concept differing_types =
	std::is_same<T, U>::value
	// make sure the serialized type member has been overloaded
	|| (T::serializedType != U::serializedType);
	*/

// all ECS objects derived from component
// TODO: now that I'm using typeid() could just attach any object
//       as a component
template <typename T>
concept is_ECSObject
	// XXX: basic check to make sure the serialized type has been overridden,
	//      need something more robust than this
	//=  differing_types<component, T>
	//&& differing_types<entity, T>
	//&& (std::is_same<T, component>::value || std::is_base_of<component, T>::value);
	= (std::is_same<T, component>::value || std::is_base_of<component, T>::value);

template <typename T>
concept Nameable =
	is_ECSObject<T>
	;
	//&& requires(T a) { a.typeString(); };

template <typename T>
concept Constructable =
	is_ECSObject<T>
	&& requires(T a) {
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

		// component maps
		// entity instances -> (attached component names -> component instances)
		std::map<entity*, std::multimap<const char *, component*>> entityComponents;
		// component names -> set of component instances
		std::map<const char *, std::set<component*>> components;
		// component instance -> entity attached to
		std::map<component*, entity*> componentEntities;
		// component instance -> set of component names registered
		//                       with the same instance
		std::map<component*, std::set<const char *>> componentTypes;

		std::set<entity*> entities;
		std::set<entity*> added;
		std::set<entity*> condemned;

		//void registerComponent(entity *ent, std::string name, component *ptr);
		template <typename T>
		void registerComponent(entity *ent, T *ptr) {
			//registerComponent(ent, T::serializedType, ptr);
			registerComponent(ent, getTypeName<T>(), ptr);
		};

		// should be called from interface constructors
		template <typename T>
		void registerInterface(entity *ent, T *ptr) {
			registerInterface(ent, getTypeName<T>(), ptr);
		}

		void unregisterComponent(entity *ent, component *ptr);
		void unregisterComponentType(entity *ent, std::string name);

		void add(entity *ent);
		void remove(entity *ent);
		bool valid(entity *ent);

		void update(float delta);
		void activate(entity *ent);
		void deactivate(entity *ent);

		entity *getEntity(component *com); 
		std::multimap<const char *, component*>& getEntityComponents(entity *ent);

		//bool hasComponents(entity *ent, std::vector<std::string> tags);

		bool hasComponents(entity *ent, std::initializer_list<const char *> tags);
		bool hasComponents(entity *ent, std::vector<const char *> tags);

		/*
		template <typename T>
		bool hasComponent(entity *ent) {
			//return hasComponents(ent, {T::serializedType});
		};
		*/

		// remove() doesn't immediately free, just adds the entity
		// to a queue of the condemned, clearFreedEntities() banishes the
		// condemned from memory
		void freeEntity(entity *ent);
		void clearFreedEntities(void);

		// TODO: "unsafe" or "internal" namespace for untemplated queries
		//       can't really make it private
		std::set<component*>& getComponents(const char *name) {
			static std::set<component*> nullret;
			return (components.count(name))? components[name] : nullret;
		}

		template <typename T>
		std::set<component*>& getComponents() {
			static std::set<component*> nullret;
			const char *name = getTypeName<T>();
			return (components.count(name))? components[name] : nullret;
		}

		gameObject::ptr root = std::make_shared<gameObject>();

		// TODO: should this be similar to inputHandlerSystem, with
		//       onCollision components?
		std::shared_ptr<std::vector<collision>> collisions
			= std::make_shared<std::vector<collision>>();

		// XXX
		gameMain *engine;

	private:
		void registerComponent(entity *ent, const char *name, component *ptr);
		void registerInterface(entity *ent, const char *name, void *ptr);
};

template <typename... U>
struct query {
	bool hasComponents(entityManager *manager, entity *ent);
};

template <typename T>
struct query<T> {
	bool hasComponents(entityManager *manager, entity *ent);
};

class component {
	public:
		static const nlohmann::json defaultProperties(void) {
			return {};
		};

		component(entityManager *manager,
		          entity *ent,
		          nlohmann::json properties = defaultProperties())
		{
			//manager->registerComponent(ent, serializedType, this);
			manager->registerComponent(ent, this);
		}

		virtual ~component();
		virtual const char* typeString(void) const { return getTypeName(*this); };
		virtual nlohmann::json serialize(entityManager *manager) { return {}; };

		// draw the imgui widgets for this component, TODO
		// also, this class needs a polymorphic member in order to upcast,
		// so this fills that role.
		virtual void drawEditorWidgets(void) { };
};

class entity : public component {
	public:
		static const nlohmann::json defaultProperties(void) {
			return {
				{"type",        "entity"},
				//{"entity-type", serializedType},
				{"entity-type", "TODO"},
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
		virtual const char* typeString(void) const { return getTypeName(*this); };
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

		entityEventSystem(std::vector<const char *> _tags) : tags(_tags) {};

		virtual ~entityEventSystem();
		virtual void onEvent(entityManager *manager, entity *ent, float delta) {};

		std::vector<const char *> tags;
};

std::set<entity*> searchEntities(entityManager *manager,
                                 std::initializer_list<const char *> tags);
std::set<entity*> searchEntities(entityManager *manager,
                                 std::vector<const char *>& tags);

entity *findNearest(entityManager *manager,
                    glm::vec3 position,
                    std::initializer_list<const char *> tags);

entity *findFirst(entityManager *manager,
                  std::initializer_list<const char *> tags);

template <Nameable T>
T* findNearest(entityManager *manager, glm::vec3 position) {
	//return (T*)findNearest(manager, position, {T::serializedType});
	return (T*)findNearest(manager, position, {getTypeName<T>()});
}

template <Nameable T>
T* findFirst(entityManager *manager) {
	return (T*)findFirst(manager, {getTypeName<T>()});
	//return (T*)findFirst(manager, {T::serializedType});
}

template <class To>
To* castEntityComponent(entityManager *m, entity *e, const char *name) {
	auto &comps = m->getEntityComponents(e);
	auto comp = comps.find(name);

	if (comp == comps.end()) {
		// TODO: error?
		return nullptr;
	}

#if GREND_BUILD_DEBUG
	// XXX: dynamic_cast in debug mode to help catch cases where you
	//      might have forgotten to set a serializedType on a derived class
	//
	//      maybe should just accept doing dynamic_cast here all the time
	//      for safety, gains from static_cast here probably aren't worth it,
	//      benchmarking needed
	auto ret = dynamic_cast<To*>(comp->second);

	assert(ret);
	return ret;

#else
	//return dynamic_cast<T>(comp->second);
	return static_cast<To*>(comp->second);
#endif
}

/*
template <class T>
T castEntityComponent(T& val, entityManager *m, entity *e, std::string name) {
	val = castEntityComponent<T>(m, e, name);
	return val;
}
*/
template <class To, class From>
To*& castEntityComponent(To*& val, entityManager *m, entity *e) {
	const char *name = getTypeName<From>();
	val = castEntityComponent<To>(m, e, name);
	return val;
}

template <class To>
To*& castEntityComponent(To*& val, entityManager *m, entity *e) {
	const char *name = getTypeName<To>();
	val = castEntityComponent<To>(m, e, name);
	return val;
}

template <class To>
To* castEntityComponent(entityManager *m, entity *e) {
	const char *name = getTypeName<To>();
	return castEntityComponent<To>(m, e, name);
}

template <class To>
To* getComponent(entityManager *m, entity *e) {
	return castEntityComponent<To>(m, e);
}

template <class To>
To* getComponent(entityManager *m, entity *e, const char *type) {
	return castEntityComponent<To>(m, e, type);
}

/*
template <Nameable T>
T* castEntityComponent(entityManager *m, entity *e) {
	return castEntityComponent<T*>(m, e, T::serializedType);
}
*/

// TODO: generalized for any iterable container type
bool intersects(std::multimap<const char *, component*>& entdata,
                std::initializer_list<const char *> test);

bool intersects(std::multimap<const char *, component*>& entdata,
                std::vector<const char *>& test);

// namespace grendx::ecs
};

#include <grend/gameMain.hpp>
