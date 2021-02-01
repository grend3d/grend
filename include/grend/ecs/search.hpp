#pragma once
#include <grend/ecs/ecs.hpp>

namespace grendx::ecs {

/**
 * Searches for entities that contain all of a given list of component types.
 *
 * This is templated in case you want to specify your own tag container
 * type, so that tag lists aren't unnecessarily copied for each query.
 * To use this, you should define your own overload of searchEntities in a header,
 * then define the implementation in a .cpp source file somewhere,
 * that way you can avoid bloating compile times.
 *
 * @param manager Manager containing the entities to search for.
 * @param iter    Iterator pair defining the beginning and end of the tags list.
 * @param size    The number of tags contained in the tags list.
 *
 * @return A set of entities that contain all of the requested components.
 */
template <typename T>
std::set<entity*> searchEntities(entityManager *manager,
                                 std::pair<typename T::iterator, typename T::iterator> iter,
                                 size_t size)
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

	if (size == 0) {
		// lol
		return {};

	} else if (size == 1) {
		auto& comps = manager->getComponents(*iter.first);

		for (auto& comp : comps) {
			ret.insert(manager->getEntity(comp));
		}

	} else {
		// TODO: could this be done without generating the sets of entities...?
		//       would need a map of component names -> entities, which would be
		//       messy since an entity can have more than one of any component...
		//       this is probably good enough, just don't search for "entity" :P
		for (auto it = iter.first; it != iter.second; it++) {
			auto& tag = *it;
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

// namespace grendx::ecs
};
