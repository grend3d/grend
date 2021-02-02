#include "area.hpp"

area::~area() {};
areaSphere::~areaSphere() {};
areaAABB::~areaAABB() {};
areaEvent::~areaEvent() {};
areaEnter::~areaEnter() {};
areaLeave::~areaLeave() {};
areaInside::~areaInside() {};
areaOutside::~areaOutside() {};
areaSystem::~areaSystem() {};

bool areaSphere::inside(entityManager *manager, entity *ent, glm::vec3 pos) {
	return glm::distance(pos, ent->node->transform.position) < radius;
}

bool areaAABB::inside(entityManager *manager, entity *ent, glm::vec3 pos) {
	// calculate difference between min/max positions, if the position is
	// inside the box bounds, all vector components should be positive
	glm::vec3 bmin = pos - ent->node->transform.position + box.min;
	glm::vec3 bmax = ent->node->transform.position + box.max - pos;
	glm::vec3 amin = min(bmin, bmax);

	return min(min(amin.x, amin.y), amin.z) > 0;
}

void areaEnter::dispatch(entityManager *manager, entity *ent, entity *other) {
	if (currentlyInside && !lastInside) {
		onEvent(manager, ent, other);
	}
}

void areaLeave::dispatch(entityManager *manager, entity *ent, entity *other) {
	if (!currentlyInside && lastInside) {
		onEvent(manager, ent, other);
	}
}

void areaInside::dispatch(entityManager *manager, entity *ent, entity *other) {
	if (currentlyInside) {
		onEvent(manager, ent, other);
	}
}

void areaOutside::dispatch(entityManager *manager, entity *ent, entity *other) {
	if (!currentlyInside) {
		onEvent(manager, ent, other);
	}
}

void areaSystem::update(entityManager *manager, float delta) {
	auto events = manager->getComponents("areaEvent");

	for (auto& evcomp : events) {
		areaEvent *ev = dynamic_cast<areaEvent*>(evcomp);
		entity *ent = manager->getEntity(ev);

		if (!ev) {
			continue;
		}

		// TODO: this isn't going to scale well, need more efficient solution...
		auto ents = searchEntities(manager, ev->tags);
		std::set<entity*> areasIn;

		for (auto& entit : ents) {
			auto comps = manager->getEntityComponents(entit);
			auto range = comps.equal_range("area");

			for (auto it = range.first; it != range.second; it++) {
				auto& [name, comp] = *it;

				if (area *ar = dynamic_cast<area*>(comp)) {
					if (ar->inside(manager, entit, ent->node->transform.position)) {
						areasIn.insert(entit);
					}
				}
			}
		}

		ev->lastInside = ev->currentlyInside;
		ev->currentlyInside = areasIn.size() != 0;

		if (areasIn.size() > 0) {
			// TODO: dispatch for every matching area?
			ev->dispatch(manager, ent, *areasIn.begin());
		}
	}
}
