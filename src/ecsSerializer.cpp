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
	return nullptr;
}

component *factories::build(entityManager *manager,
                            entity *ent,
                            json serialized)
{
	return nullptr;
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
