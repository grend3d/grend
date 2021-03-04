#include <grend/gameObject.hpp>
#include <grend/gameMain.hpp>
#include <grend/ecs/ecs.hpp>
#include <grend/ecs/rigidBody.hpp>
#include <grend/ecs/collision.hpp>
#include <grend/ecs/serializer.hpp>

using namespace grendx;
using namespace grendx::ecs;
using namespace nlohmann;

abstractFactory::~abstractFactory() {};

entity *factories::build(entityManager *manager,
                         json serialized)
{
	auto components = serialized.find("components");
	auto enttype    = serialized.find("entity-type");
	auto node       = serialized.find("node");
	auto type       = serialized.find("type");

	bool complete =
		components != serialized.end()
		&& enttype != serialized.end()
		&& node    != serialized.end()
		&& type    != serialized.end();

	if (!complete) {
		return nullptr;
	}

	// TODO: exception handling
	std::string typestr = enttype->get<std::string>();
	SDL_Log("Build entity: %s", typestr.c_str());

	entity *ret = nullptr;

	if (has(typestr)) {
		component *built = factories[typestr]->allocate(manager, nullptr, serialized);
		ret = dynamic_cast<entity*>(built);
	}

	if (!ret) {
		return nullptr;
	}

	for (auto& comp : *components) {
		build(manager, ret, comp);
	}

	return ret;
}

component *factories::build(entityManager *manager,
                            entity *ent,
                            json serialized)
{
	if (!serialized.is_array() /*|| serialized.size() < 2*/) {
		return nullptr;
	}

	std::string type = serialized[0].get<std::string>();
	SDL_Log("Attempting to add component %s to entity %p...",
	        type.c_str(), ent);

	if (!has(type)) {
		return nullptr;
	}

	SDL_Log("Found component type %s", type.c_str());

	nlohmann::json props = serialized[1].is_null()
		? properties(type)
		: serialized[1];

	return factories[type]->allocate(manager, ent, props);
}


/*
json serializerRegistry::serializeEntity(entityManager *manager, entity *ent) {
	std::map<component*, std::set<std::string>> compmap;
	auto components = manager->getEntityComponents(ent);

	// TODO: I don't think there's a way to avoid this, but it is kind of a bummer
	//       that this is what, MN log N? Serialization is going to be heavy
	//       no matter what I guess
	for (auto& [name, comp] : components) {
		compmap[comp].insert(name);
	}

	json compJson;
	for (auto& [comp, names] : compmap) {
		json compClasses;

		for (auto& name : names) {
			auto it = serializers.find(name);
			if (it != serializers.end()) {
				auto& [name, ser] = *it;
				compClasses.emplace(name, ser->serialize(comp));
			}
		}

		compJson.push_back({{
			"component",
			{
				{"type",    comp->typeString()},
				{"classes", compClasses},
			}
		}});
	}

	return {"entity", compJson};
}

json serializerRegistry::serializeEntities(entityManager *manager) {
	json entities;

	for (auto& ent : manager->entities) {
		entities.push_back(serializeEntity(manager, ent));
	}

	return entities;
}

void serializerRegistry::deserializeEntity(entityManager *manager, entity *ent) {
	// TODO:
}
*/
