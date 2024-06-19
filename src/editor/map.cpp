#include <grend/ecs/ecs.hpp>
#include <grend/ecs/serializer.hpp>
#include <grend/ecs/sceneComponent.hpp>

#include <grend/gameEditor.hpp>
#include <grend/loadScene.hpp>
#include <grend/utility.hpp>
#include <grend/logger.hpp>
#include <iostream>
#include <fstream>

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_sdl.h>
#include <imgui/backends/imgui_impl_opengl3.h>

#include <nlohmann/json.hpp>

using namespace grendx;
using namespace grendx::ecs;
using namespace grendx::engine;
using namespace nlohmann;

auto links(ecs::entity *ent) {
	return ent->getAll<ecs::baseLink>();
}

static inline std::vector<float> strvec_to_float(std::vector<std::string> strs) {
	std::vector<float> ret;

	for (auto& s : strs) {
		if (!s.empty()) {
			ret.push_back(std::stof(s));
		}
	}

	return ret;
}

template <typename T>
static inline T parse_vec(std::string& str) {
	auto temp = strvec_to_float(split_string(str, ','));
	// TODO: throw an exception here?
	assert(temp.size() >= T::length());

	T ret;
	for (unsigned i = 0; i < T::length(); i++) {
		ret[i] = temp[i];
	}

	return ret;
}

static modelMap xxx_load_model(std::string filename, std::string objName) {
	auto ext = filename_extension(filename);
	modelMap models;

	if (ext == ".obj") {
		auto model = load_object(filename);
		models[objName] = model;

	} else if (ext == ".gltf") {
		models = load_gltf_models(filename);
	}

	return models;
}

// TODO: set to keep track of map files already being loaded to avoid
//       recursive map loads
sceneNode::ptr loadNodes(std::map<std::string, modelMap>& sources,
                         json js,
                         bool lowerLevel = false)
{
	// TODO: avoid redundant resolves, only need to do resolves at the top level
	auto ecs       = Resolve<ecs::entityManager>();
	auto factories = Resolve<ecs::serializer>();

	sceneNode::ptr ret = nullptr;
	bool recurse = true;

	ecs::entity *ent = factories->build(ecs, js);

	if (!ent) {
		LogError("Couldn't load entity");
		std::cerr<< js.dump(4) << std::endl;
		return nullptr;
	}

	sceneNode* imp = dynamic_cast<sceneNode*>(ent);
	// TODO: source link
	LogInfo((std::string)js.dump(4));

	bool haveSourceFile = js.contains("entity-properties")
		&& js["entity-properties"].contains("sourceFile");

	if (lowerLevel && haveSourceFile) {
		// backward compatibility for old map files (for now)
		// TODO: remove this at some point, it's just for my own convenience
		std::string sourceFile = js["entity-properties"]["sourceFile"];
		LogFmt("Got here, have a legacy source file: '{}'", sourceFile);

		imp->attach<sceneComponent>(sourceFile, sceneComponent::Usage::Copy);

		// TODO: FIXME: previously built entity is unused here, ends up leaking
		recurse = false;
		ret = imp;

	} else if (imp->has<sceneComponent>()) {
		ret     = imp;
		recurse = false;

	} else if (sceneNode *temp = dynamic_cast<sceneNode*>(ent)) {
		ret = ecs::ref(temp);

	} else {
		LogErrorFmt("Entity is not a sceneNode: {}", ent->typeString());
	}

	if (ret && recurse && !js["nodes"].is_null()) {
		for (auto& ptr : js["nodes"]) {
			// TODO: don't assign new name
			static unsigned counter = 0;
			std::string name;

			if (ptr.contains("entity-properties")
			    && ptr["entity-properties"].contains("name")
			    && ptr["entity-properties"]["name"].is_string()
			    && ptr["entity-properties"]["name"].size() > 0)
			{
				name = ptr["entity-properties"]["name"];

			} else {
				name = "node" + std::to_string(counter++);
			}

			auto node = loadNodes(sources, ptr, true);
			setNode(name, ret, node);
		}
	}

	if (ret) {
		// XXX: need to call this to update cached uniforms after entity is constructed
		ret->transform.set(ret->transform.getTRS());
	}

	return ret;
}

result<objectPair>
grendx::loadMapData(std::string name) noexcept {
	std::ifstream foo(name);
	LogFmt("loading map {}", name);

	if (!foo.good()) {
		std::string asdf = "couldn't open map file: " + name;
		return invalidResult(asdf);
	}

	try {
		auto ecs = engine::Resolve<ecs::entityManager>();
		sceneNode::ptr ret = ecs->construct<sceneNode>();
		// TODO: source link
		json j;
		foo >> j;

		// XXX: again TODO
		modelMap retmodels;
		std::map<std::string, modelMap> sources;

		sceneNode::ptr temp = loadNodes(sources, j["root"]);
		ret->transform.set(temp->transform.getTRS());

		for (auto link : temp->nodes()) {
			if (auto ptr = link->getRef()) {
				setNode(ptr->name, ret, ptr);
			}
		}

		for (auto ptr : ret->nodes()) {
			(*ptr)->parent = ret;
		}

		for (auto& [name, ptr] : sources) {
			retmodels.insert(ptr.begin(), ptr.end());
		}

		return objectPair {ret, retmodels};

	} catch (std::exception& e) {
		LogErrorFmt("loadMap(): couldn't parse {}: {}", name, e.what());
		return invalidResult(e.what());
	}
}

grendx::result<sceneNode::ptr>
grendx::loadMapCompiled(std::string name) noexcept {
	if (auto res = loadMapData(name)) {
		auto [obj, models] = *res;
		compileModels(models);
		return obj;

	} else {
		return invalidResult(res.what());
	}
}

template <typename T>
static inline std::string format_vec(T& vec) {
	std::string ret = "";

	for (unsigned i = 0; i < T::length(); i++) {
		ret += std::to_string(vec[i]) + ",";
	}

	return ret;
}

template <typename T>
static inline std::string format_mat(T& mtx) {
	std::string ret = "";

	for (unsigned y = 0; y < T::col_type::length(); y++) {
		for (unsigned x = 0; x < T::row_type::length(); x++) {
			ret += std::to_string(mtx[y][x]) + ",";
		}
	}

	return ret;
}

static json objectJson(sceneNode::ptr obj, bool toplevel = false) {
	auto factories = Resolve<ecs::serializer>();
	auto entities  = Resolve<ecs::entityManager>();

	bool recurse = toplevel || (obj->settings & ecs::entity::serializeLinks);
	auto curjson = factories->serialize(entities, obj.getPtr());

	json ret = factories->serialize(entities, obj.getPtr());

	if (recurse) {
		json nodes = {};

		for (auto ptr : obj->nodes()) {
			nodes.push_back(objectJson(ptr->getRef()));
		}

		ret["nodes"] = nodes;
	}

	return ret;
}

void grendx::saveMap(sceneNode::ptr root,
                     std::string name) noexcept
{
	std::ofstream foo(name);
	LogFmt("saving map {}", name);

	if (!foo.good()) {
		LogErrorFmt("couldn't open save file {}", name);
		return;
	}

	json j = {{
		"map", {
			{"file", name},
		}}};

	j["root"] = objectJson(root, true /*toplevel*/);

	foo << j.dump(4) << std::endl;
}
