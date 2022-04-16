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

nlohmann::json serializer::serialize(entityManager *manager, entity *ent) {
	for (auto [name, _] : factories) {
		std::cout << "got a " << name << std::endl;
	}
	using namespace nlohmann;

	json components = {};
	auto uniq = ent->getAll<component>();

	for (auto it = uniq.first; it != uniq.second; it++) {
		auto& [_, comp] = *it;

		// entities have themselves as components, don't recurse infinitely
		if (comp != ent) {
			json props = json::object();;

			for (auto& subtype : manager->componentTypes[comp]) {
				const std::string& demangled = demangle(subtype);

				if (has(demangled)) {
					json temp = serializers[demangled](comp);
					props.insert(temp.begin(), temp.end());
				}
			}

			components.push_back({ demangle(comp->typeString()), props });
		}
	}

	json entprops = json::object();
	for (auto& subtype : manager->componentTypes[ent]) {
		const std::string& demangled = demangle(subtype);

		if (has(demangled)) {
			std::cout << "got here, serializing a " << demangled << std::endl;
			json temp = serializers[demangled](ent);
			entprops.insert(temp.begin(), temp.end());
		}
	}

	return {
		{"entity-type",       demangle(ent->typeString())},
		{"entity-properties", entprops},
		{"components",        components},
	};
}

entity *serializer::build(entityManager *manager, json serialized)
{
	auto components = serialized.find("components");
	auto enttype    = serialized.find("entity-type");
	auto entprops   = serialized.find("entity-properties");
	//auto node       = serialized.find("node");
	//auto type       = serialized.find("type");

	bool complete =
		components  != serialized.end()
		&& enttype  != serialized.end()
		&& entprops != serialized.end()
		//&& node     != serialized.end()
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
		const std::string& demangled = demangle(subtype);

		if (deserializers.contains(demangled)) {
			deserializers[demangled](ret, *entprops);
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

	// unmangled name stored in serialized form
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
		// indexing into component names registered under the component,
		// need to demangle to search for suitable deserializers
		// TODO: should probably warn (or have an option for warning, set by default)
		//       if a subcomponent wasn't found
		const std::string& demangled = demangle(subtype);

		if (deserializers.contains(demangled)) {
			deserializers[demangled](ret, serialized[1]);
		}
	}

	return ret;

}
