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
using namespace nlohmann;

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

			return scenes[source];
		}

		std::map<std::string, modelMap> sources;
		std::map<std::string, sceneImport::ptr> scenes;
};

// TODO: set to keep track of map files already being loaded to avoid
//       recursive map loads
sceneNode::ptr loadNodes(modelCache& cache, std::string name, json jay) {
	sceneNode::ptr ret = nullptr;
	std::vector<std::string> modelFiles;
	bool recurse = true;

	if (jay["type"] == "Imported file") {
		if (!jay["sourceFile"].is_null()) {
			recurse = false;

			if ((ret = cache.getScene(jay["sourceFile"])) == nullptr) {
				LogErrorFmt("loadMap(): Unknown model {}", jay["sourceFile"]);
				ret = std::make_shared<sceneNode>();
			}

		} else {
			// XXX: top-level map import nodes are exported without a sourceFile,
			//      so re-import without a source file set, the source file will
			//      be set in the loadMap* functions
			ret = std::make_shared<sceneImport>("");
		}

	} else if (jay["type"] == "Model") {
		recurse = false;

		if ((ret = cache.getModel(jay["sourceFile"], name)) == nullptr) {
			LogErrorFmt("loadMap(): Unknown model {}", name);
			ret = std::make_shared<sceneNode>();
		}

	} else if (jay["type"] == "Point light") {
		auto light = std::make_shared<sceneLightPoint>();
		auto& diff = jay["diffuse"];

		light->diffuse = glm::vec4(diff[0], diff[1], diff[2], diff[3]);
		light->intensity = jay["intensity"];
		light->casts_shadows = jay["casts_shadows"];
		light->is_static = jay["is_static"];
		light->radius = jay["radius"];

		ret = std::static_pointer_cast<sceneNode>(light);

	} else if (jay["type"] == "Spot light") {
		auto light = std::make_shared<sceneLightSpot>();
		auto& diff = jay["diffuse"];

		light->diffuse = glm::vec4(diff[0], diff[1], diff[2], diff[3]);
		light->intensity = jay["intensity"];
		light->casts_shadows = jay["casts_shadows"];
		light->is_static = jay["is_static"];
		light->radius = jay["radius"];
		light->angle  = jay["angle"];

		ret = std::static_pointer_cast<sceneNode>(light);

	} else if (jay["type"] == "Directional light") {
		auto light = std::make_shared<sceneLightDirectional>();
		auto& diff = jay["diffuse"];

		light->diffuse = glm::vec4(diff[0], diff[1], diff[2], diff[3]);
		light->intensity = jay["intensity"];
		light->casts_shadows = jay["casts_shadows"];
		light->is_static = jay["is_static"];

		ret = std::static_pointer_cast<sceneNode>(light);

	} else if (jay["type"] == "Reflection probe"){
		auto probe = std::make_shared<sceneReflectionProbe>();
		auto bbox = jay["boundingBox"];

		auto bmin = bbox["min"];
		auto bmax = bbox["max"];

		probe->is_static = jay["is_static"];
		probe->boundingBox.min = glm::vec3(bmin[0], bmin[1], bmin[2]);
		probe->boundingBox.max = glm::vec3(bmax[0], bmax[1], bmax[2]);
		ret = std::static_pointer_cast<sceneNode>(probe);

	} else if (jay["type"] == "Irradiance probe"){
		auto probe = std::make_shared<sceneIrradianceProbe>();
		auto bbox = jay["boundingBox"];

		auto bmin = bbox["min"];
		auto bmax = bbox["max"];

		probe->is_static = jay["is_static"];
		probe->boundingBox.min = glm::vec3(bmin[0], bmin[1], bmin[2]);
		probe->boundingBox.max = glm::vec3(bmax[0], bmax[1], bmax[2]);
		ret = std::static_pointer_cast<sceneNode>(probe);

	} else {
		ret = std::make_shared<sceneNode>();
	}

	auto& pos   = jay["position"];
	auto& scale = jay["scale"];
	auto& rot   = jay["rotation"];

	TRS newtrans = ret->getTransformTRS();
	newtrans.position = glm::vec3(pos[0], pos[1], pos[2]);
	newtrans.scale    = glm::vec3(scale[0], scale[1], scale[2]);
	newtrans.rotation = glm::quat(rot[0], rot[1], rot[2], rot[3]);
	ret->setTransform(newtrans);

	if (recurse && !jay["nodes"].is_null()) {
		for (auto& [name, ptr] : jay["nodes"].items()) {
			if (!ptr.is_null()) {
				setNode(name, ret, loadNodes(cache, name, ptr));
			}
		}
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
		sceneImport::ptr ret = std::make_shared<sceneImport>(name);
		json j;
		foo >> j;

		// XXX: again TODO
		modelCache cache;
		modelMap retmodels;

		sceneNode::ptr temp = loadNodes(cache, "", j["root"]);
		ret->setTransform(temp->getTransformTRS());
		ret->nodes = temp->nodes;

		for (auto& [_, ptr] : ret->nodes) {
			ptr->parent = ret;
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
	bool recurse = true;
	TRS transform = obj->getTransformTRS();

	json ret = {
		{"type",     obj->typeString()},
		{"position", {transform.position.x,
		              transform.position.y,
		              transform.position.z}},
		{"scale",    {transform.scale.x,
		              transform.scale.y,
		              transform.scale.z}},
		{"rotation", {transform.rotation.w,
		              transform.rotation.x,
		              transform.rotation.y,
		              transform.rotation.z}},
	};

	// XXX: toplevel flag to avoid empty recursive saves when saving an
	//      imported map file
	if (obj->type == sceneNode::objType::Import && !toplevel) {
		sceneImport::ptr imported = std::static_pointer_cast<sceneImport>(obj);
		ret["sourceFile"] = imported->sourceFile;
		recurse = false;

	} else if (obj->type == sceneNode::objType::Model) {
		sceneModel::ptr model = std::static_pointer_cast<sceneModel>(obj);
		ret["sourceFile"] = model->sourceFile;
		recurse = false;

	} else if (obj->type == sceneNode::objType::ReflectionProbe) {
		auto probe = std::static_pointer_cast<sceneReflectionProbe>(obj);
		auto bmin = probe->boundingBox.min;
		auto bmax = probe->boundingBox.max;

		ret["boundingBox"] = {
			{"min", {bmin[0], bmin[1], bmin[2]}},
			{"max", {bmax[0], bmax[1], bmax[2]}},
		};
		ret["is_static"] = probe->is_static;

	} else if (obj->type == sceneNode::objType::IrradianceProbe) {
		auto probe = std::static_pointer_cast<sceneIrradianceProbe>(obj);
		auto bmin = probe->boundingBox.min;
		auto bmax = probe->boundingBox.max;

		ret["boundingBox"] = {
			{"min", {bmin[0], bmin[1], bmin[2]}},
			{"max", {bmax[0], bmax[1], bmax[2]}},
		};
		ret["is_static"] = probe->is_static;

	} else if (obj->type == sceneNode::objType::Light) {
		sceneLight::ptr light = std::static_pointer_cast<sceneLight>(obj);

		auto& d = light->diffuse;
		ret["diffuse"] = {d[0], d[1], d[2], d[3]};
		ret["intensity"] = light->intensity;
		ret["casts_shadows"] = light->casts_shadows;
		ret["is_static"] = light->is_static;

		if (light->lightType == sceneLight::lightTypes::Point) {
			sceneLightPoint::ptr p = std::static_pointer_cast<sceneLightPoint>(obj);
			ret["radius"] = p->radius;
		}

		else if (light->lightType == sceneLight::lightTypes::Spot) {
			sceneLightSpot::ptr p = std::static_pointer_cast<sceneLightSpot>(obj);
			ret["radius"] = p->radius;
			ret["angle"]  = p->angle;
		}

		else if (light->lightType == sceneLight::lightTypes::Directional) {
			// only base light members
		}

		// TODO: other lights
	}

	if (recurse) {
		json nodes = {};
		for (auto& [name, ptr] : obj->nodes) {
			nodes[name] = objectJson(ptr);
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
