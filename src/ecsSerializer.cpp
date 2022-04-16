#include <grend/sceneNode.hpp>
#include <grend/gameMain.hpp>
#include <grend/ecs/ecs.hpp>
#include <grend/ecs/rigidBody.hpp>
#include <grend/ecs/collision.hpp>
#include <grend/ecs/serializer.hpp>

using namespace grendx;
using namespace grendx::ecs;
using namespace nlohmann;

abstractFactory::~abstractFactory() {};

entity *serializer::build(entityManager *manager, json serialized)
{
	auto components = serialized.find("components");
	auto enttype    = serialized.find("entity-type");
	auto entprops   = serialized.find("entity-properties");
	auto node       = serialized.find("node");
	//auto type       = serialized.find("type");

	bool complete =
		components  != serialized.end()
		&& enttype  != serialized.end()
		&& entprops != serialized.end()
		&& node     != serialized.end()
		/*&& type     != serialized.end()*/
		;

	if (!complete) {
		return nullptr;
	}

	// TODO: exception handling
	std::string typestr = enttype->get<std::string>();
	SDL_Log("Build entity: %s", typestr.c_str());

	entity *ret = nullptr;

	if (has(typestr)) {
		component *built = factories[typestr]->allocate(manager, nullptr);
		if ((ret = dynamic_cast<entity*>(built)) == nullptr) {
			// TODO: need to delete 'built' and return,
			//       given type was not an entity type
		}
	}

	if (!ret) {
		return nullptr;
	}

	for (auto& subtype : manager->componentTypes[ret]) {
		if (deserializers.contains(subtype)) {
			deserializers[subtype](ret, *entprops);
		}
		//factories[subtype].
	}

	for (auto& comp : *components) {
		build(manager, ret, comp);
	}

	return ret;
}

component *serializer::build(entityManager *manager,
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

	component *ret = factories[type]->allocate(manager, ent);
	/*
	nlohmann::json props = serialized[1].is_null()
		? properties(type)
		: serialized[1];
		*/

	for (auto& subtype : manager->componentTypes[ret]) {
		if (deserializers.contains(subtype)) {
			deserializers[subtype](ret, serialized[1]);
		}
	}

	return ret;

}
