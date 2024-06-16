#include <grend/ecs/ecs.hpp>
#include <grend/ecs/serializer.hpp>

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

static
sceneImport::ptr cloneImport(sceneImport::ptr scene) {
	if (!scene)
		return scene;

	auto ecs = Resolve<ecs::entityManager>();
	sceneImport::ptr ret = ecs->construct<sceneImport>(scene->sourceFile);
	ret->transform.set(scene->transform.getTRS());

	for (auto link : scene->nodes()) {
		auto ptr = link->getRef();
		auto& name = ptr->name;
		setNode(name, ret, ptr);
	}

	return ret;
}

// XXX: TODO: move to model loading code
class modelCache {
	public:
		sceneModel::ptr getModel(std::string source, std::string name) {
			if (sources.find(source) == sources.end()) {
				sources[source] = xxx_load_model(source, name);
			}

			modelMap& models = sources[source];

			// hmm... just added it to the map no?
			return (models.find(name) != models.end())
				? models[name]
				: nullptr;
		}

		sceneImport::ptr getScene(std::string source) {
			if (scenes.find(source) == scenes.end()) {
				if (auto scene = loadSceneData(source)) {
					auto [objs, models] = *scene;
					scenes[source]  = objs;
					sources[source] = models;

				} else {
					printError(scene);
					scenes[source] = nullptr;
				}
			}

			return cloneImport(scenes[source]);
		}

		std::map<std::string, modelMap> sources;
		std::map<std::string, sceneImport::ptr> scenes;
};

// TODO: set to keep track of map files already being loaded to avoid
//       recursive map loads
sceneNode::ptr loadNodes(modelCache& cache,
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

	sceneImport* imp = dynamic_cast<sceneImport*>(ent);
	if (lowerLevel && imp) {
		// TODO: FIXME: previously built entity is unused here, ends up leaking
		recurse = false;

		ret = imp;
		sceneImport::ptr tempNode = cache.getScene(imp->sourceFile);

		if (tempNode == nullptr) {
			LogErrorFmt("loadMap(): Unknown model {}", (std::string)js["sourceFile"]);
			ret = ecs->construct<sceneNode>();

		} else {
			for (auto link : tempNode->nodes()) {
				auto node = link->getRef();
				setNode(node->name, ret, node);
			}

			imp->animations = tempNode->animations;
		}

		// XXX: reapply entity transforms to sceneImport
		ecs::entity::deserializer(ret.getPtr(), js["entity-properties"]);

	} else if (sceneNode *temp = dynamic_cast<sceneNode*>(ent)) {
		ret = ecs::ref(temp);

	} else {
		LogErrorFmt("Entity is not a sceneNode: {}", ent->typeString());
	}

	if (ret && recurse && !js["nodes"].is_null()) {
		for (auto& ptr : js["nodes"]) {
			// TODO: don't assign new name
			static unsigned counter = 0;
			std::string name = "node" + std::to_string(counter++);

			auto node = loadNodes(cache, ptr, true);
			setNode(name, ret, node);
		}
	}

	if (ret) {
		// XXX: need to call this to update cached uniforms after entity is constructed
		ret->transform.set(ret->transform.getTRS());
	}

	return ret;
}

result<importPair>
grendx::loadMapData(std::string name) noexcept {
	std::ifstream foo(name);
	LogFmt("loading map {}", name);

	if (!foo.good()) {
		std::string asdf = "couldn't open map file: " + name;
		return invalidResult(asdf);
	}

	try {
		auto ecs = engine::Resolve<ecs::entityManager>();
		sceneImport::ptr ret = ecs->construct<sceneImport>(name);
		json j;
		foo >> j;

		// XXX: again TODO
		modelCache cache;
		modelMap retmodels;

		sceneNode::ptr temp = loadNodes(cache, j["root"]);
		ret->transform.set(temp->transform.getTRS());

		for (auto link : temp->nodes()) {
			if (auto ptr = link->getRef()) {
				setNode(ptr->name, ret, ptr);
			}
		}

		for (auto ptr : ret->nodes()) {
			(*ptr)->parent = ret;
		}

		for (auto& [name, ptr] : cache.sources) {
			retmodels.insert(ptr.begin(), ptr.end());
		}

		return importPair {ret, retmodels};

	} catch (std::exception& e) {
		LogErrorFmt("loadMap(): couldn't parse {}: {}", name, e.what());
		return invalidResult(e.what());
	}
}

grendx::result<sceneImport::ptr>
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
