/// @file
// TODO: more doxygen, copyright, etc
#pragma once

#include <grend/sceneNode.hpp>
#include <grend/physics.hpp>

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

template <typename T, typename... Us>
const char *getFirstTypeName() {
	return getTypeName<T>();
}

template <typename... T>
std::array<const char *, sizeof...(T)> getTypeNames() {
	return { getTypeName<T>()... };
}

// TODO: should define map types as part of entityManager
using CompMap = std::multimap<const char *, component*>;

// assumes that the entity ptr is valid
template <typename T, typename... Us>
struct matchesType {

	bool operator()(entity *ptr, const CompMap& compmap) const {
		return compmap.contains(getTypeName<T>())
			&& matchesType<Us...>{}(ptr, compmap);
	}
};

template <typename T>
struct matchesType<T> {
	bool operator()(entity *ptr, const CompMap& compmap) const {
		return compmap.contains(getTypeName<T>());
	}
};

template <typename... T>
struct searchIterator;

template <typename... T>
struct searchResults;

template <typename... T>
searchResults<T...> searchEntities(entityManager *manager);

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
		template <typename T, typename U>
		void registerInterface(entity *ent, U *ptr) noexcept {
			static_assert(std::is_base_of<component, U>::value,
			              "Given component type must be derived from ecs::component");
			static_assert(std::is_base_of<T, U>::value,
			              "Given component type must implement the interface specified");
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

		template <typename... T>
		searchResults<T...> search() {
			return searchEntities<T...>(this);
		}

		template <typename... T>
		bool hasComponents(entity *ent) {
			return matchesType<T...>{}(ent, getEntityComponents(ent));
		}

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

		sceneNode::ptr root = std::make_shared<sceneNode>();

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

class component {
	public:
		component(entityManager *_manager, entity *ent)
			: manager(_manager)
		{
			manager->registerComponent(ent, this);
		}

		virtual ~component();
		virtual const char* typeString(void) const { return getTypeName(*this); };

		// draw the imgui widgets for this component, TODO
		// also, this class needs a polymorphic member in order to upcast,
		// so this fills that role.
		virtual void drawEditorWidgets(void) { };

		entityManager *manager;
};

class entity : public component {
	public:
		typedef std::shared_ptr<entity> ptr;
		typedef std::weak_ptr<entity>   weakptr;

		entity(entityManager *manager);
		entity(entityManager *manager, entity *ent);

		virtual ~entity();
		virtual const char* typeString(void) const { return getTypeName(*this); };
		virtual sceneNode::ptr getNode(void) { return node; };

		template <typename T, typename... Args>
		T* attach(Args... args) {
			return new T(manager, this, args...);
		}

		template <typename T>
		T* get() {
			return getComponent<T>(manager, this);
		}

		template <typename T>
		std::pair<std::multimap<const char *, component*>::iterator,
				  std::multimap<const char *, component*>::iterator>
		getAll()
		{
			auto& compmap = manager->getEntityComponents(this);
			return compmap.equal_range(getTypeName<T>());
		}

		template <typename T>
		T* getInterface() {
			return getInterface<T>(manager, this);
		}

		sceneNode::ptr node = std::make_shared<sceneNode>();
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

template <typename T>
T* findNearest(entityManager *manager, glm::vec3 position) {
	return (T*)findNearest(manager, position, {getTypeName<T>()});
}

template <typename T>
T* findFirst(entityManager *manager) {
	return (T*)findFirst(manager, {getTypeName<T>()});
}

template <typename... T>
entity *findNearestTypes(entityManager *manager, glm::vec3 position) {
	return findNearest(manager, position, { getTypeName<T>()... });
}

template <typename... T>
entity *findFirstTypes(entityManager *manager) {
	return findFirst(manager, { getTypeName<T>()... });
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

template <typename... T>
struct searchIterator {
	using IterType = std::set<component*>::iterator;
	IterType it;
	IterType end;

	searchIterator(IterType startit, IterType endit)
		: it(startit), end(endit)
	{
		// find first result to return
		searchToNextMatch();
	};

	void searchToNextMatch(void) {
		if (it != end && sizeof...(T) > 1) {
			entity *ent = (*it)->manager->getEntity(*it);
			const auto& compmap = ent->manager->getEntityComponents(ent);

			while (!matchesType<T...>{}(ent, compmap)) {
				it++;
				if (it == end)
					break;

				ent = (*it)->manager->getEntity(*it);
			}
		}
	}

	const searchIterator& operator++(void) {
		if (it != end) {
			it++;
			searchToNextMatch();
		}

		return *this;
	}

	entity *operator*(void) const {
		if (it == end) {
			return nullptr;
		}

		component *comp = *it;
		return comp->manager->getEntity(comp);
	}

	bool operator==(const searchIterator& other) const {
		return it == other.it;
	}

	bool operator!=(const searchIterator& other) const {
		//return !(*this == other);
		return it != other.it;
	}
};

template <typename... T>
struct searchResults {
	std::set<component*>::iterator it;
	std::set<component*>::iterator endit;

	searchIterator<T...> begin() {
		return searchIterator<T...>(it, endit);
	}

	searchIterator<T...> end() {
		return searchIterator<T...>(endit, endit);
	}
};

// TODO: documentation
// O(mn log p) time, O(1) storage
// where m is the number of entities in the smallest entity group,
// n is the number of types being searched
// p is the number of entities that contain each type
// equivalent to getComponents(type) when called with one type
template <typename... T>
searchResults<T...> searchEntities(entityManager *manager) {
	// find smallest (most exclusive) set of candidates
	size_t curmin = UINT_MAX;
	auto temp = getTypeNames<T...>();

	searchResults<T...> ret;

	for (const char *str : temp) {
		std::set<component*>& comps = manager->getComponents(str);

		if (comps.size() < curmin) {
			ret.it    = comps.begin();
			ret.endit = comps.end();
			curmin    = comps.size();
		}
	}

	return ret;
}

// namespace grendx::ecs
};

#include <grend/gameMain.hpp>
