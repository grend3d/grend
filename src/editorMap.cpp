#include <grend/gameEditor.hpp>
#include <grend/utility.hpp>
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
		gameModel::ptr getModel(std::string source, std::string name) {
			if (sources.find(source) == sources.end()) {
				sources[source] = xxx_load_model(source, name);
			}

			modelMap& models = sources[source];

			// hmm... just added it to the map no?
			return (models.find(name) != models.end())
				? models[name]
				: nullptr;
		}

		gameImport::ptr getScene(std::string source) {
			if (scenes.find(source) == scenes.end()) {
				auto [objs, models] = loadSceneData(source);
				scenes[source]  = objs;
				sources[source] = models;
			}

			return scenes[source];
		}

		std::map<std::string, modelMap> sources;
		std::map<std::string, gameImport::ptr> scenes;
};

gameObject::ptr loadNodes(modelCache& cache, std::string name, json jay) {
	gameObject::ptr ret = nullptr;
	std::vector<std::string> modelFiles;
	bool recurse = true;

	if (jay["type"] == "Imported file") {
		// TODO: scene cache
		recurse = false;

		if ((ret = cache.getScene(jay["sourceFile"])) == nullptr) {
			std::cerr << "loadMap(): Unknown model "
				<< jay["sourceFile"] << std::endl;
			ret = std::make_shared<gameObject>();
		}

	} else if (jay["type"] == "Model") {
		recurse = false;

		if ((ret = cache.getModel(jay["sourceFile"], name)) == nullptr) {
			std::cerr << "loadMap(): Unknown model " << name << std::endl;
			ret = std::make_shared<gameObject>();
		}

	} else if (jay["type"] == "Point light") {
		auto light = std::make_shared<gameLightPoint>();
		auto& diff = jay["diffuse"];

		light->diffuse = glm::vec4(diff[0], diff[1], diff[2], diff[3]);
		light->intensity = jay["intensity"];
		light->casts_shadows = jay["casts_shadows"];
		light->is_static = jay["is_static"];
		light->radius = jay["radius"];

		ret = std::dynamic_pointer_cast<gameObject>(light);

	} else if (jay["type"] == "Spot light") {
		auto light = std::make_shared<gameLightSpot>();
		auto& diff = jay["diffuse"];

		light->diffuse = glm::vec4(diff[0], diff[1], diff[2], diff[3]);
		light->intensity = jay["intensity"];
		light->casts_shadows = jay["casts_shadows"];
		light->is_static = jay["is_static"];
		light->radius = jay["radius"];
		light->angle  = jay["angle"];

		ret = std::dynamic_pointer_cast<gameObject>(light);

	} else if (jay["type"] == "Directional light") {
		auto light = std::make_shared<gameLightDirectional>();
		auto& diff = jay["diffuse"];

		light->diffuse = glm::vec4(diff[0], diff[1], diff[2], diff[3]);
		light->intensity = jay["intensity"];
		light->casts_shadows = jay["casts_shadows"];
		light->is_static = jay["is_static"];

		ret = std::dynamic_pointer_cast<gameObject>(light);

	} else if (jay["type"] == "Reflection probe"){
		auto probe = std::make_shared<gameReflectionProbe>();
		auto bbox = jay["boundingBox"];

		auto bmin = bbox["min"];
		auto bmax = bbox["max"];

		probe->is_static = jay["is_static"];
		probe->boundingBox.min = glm::vec3(bmin[0], bmin[1], bmin[2]);
		probe->boundingBox.max = glm::vec3(bmax[0], bmax[1], bmax[2]);
		ret = std::dynamic_pointer_cast<gameObject>(probe);

	} else if (jay["type"] == "Irradiance probe"){
		auto probe = std::make_shared<gameIrradianceProbe>();
		auto bbox = jay["boundingBox"];

		auto bmin = bbox["min"];
		auto bmax = bbox["max"];

		probe->is_static = jay["is_static"];
		probe->boundingBox.min = glm::vec3(bmin[0], bmin[1], bmin[2]);
		probe->boundingBox.max = glm::vec3(bmax[0], bmax[1], bmax[2]);
		ret = std::dynamic_pointer_cast<gameObject>(probe);

	} else {
		ret = std::make_shared<gameObject>();
	}

	auto& pos   = jay["position"];
	auto& scale = jay["scale"];
	auto& rot   = jay["rotation"];

	//std::cerr << jay.dump(3) << std::endl;

	ret->transform.position = glm::vec3(pos[0], pos[1], pos[2]);
	ret->transform.scale    = glm::vec3(scale[0], scale[1], scale[2]);
	ret->transform.rotation = glm::quat(rot[0], rot[1], rot[2], rot[3]);

	if (recurse && !jay["nodes"].is_null()) {
		for (auto& [name, ptr] : jay["nodes"].items()) {
			if (!ptr.is_null()) {
				setNode(name, ret, loadNodes(cache, name, ptr));
			}
		}
	}

	return ret;
}

std::pair<gameObject::ptr, modelMap>
grendx::loadMapData(gameMain *game, std::string name) {
	std::ifstream foo(name);
	std::cerr << "loading map " << name << std::endl;

	if (!foo.good()) {
		std::cerr << "couldn't open save file" << name << std::endl;
		return {std::make_shared<gameObject>(), {}};
	}

	try {
		gameObject::ptr ret = nullptr;
		json j;
		foo >> j;

		// XXX: again TODO
		modelCache cache;
		modelMap retmodels;
		ret = loadNodes(cache, "", j["root"]);

		for (auto& [name, ptr] : cache.sources) {
			// TODO: prepend with name to avoid collisions
			//       also don't need name info anymore for compiling,
			//       could just return a model set
			retmodels.insert(ptr.begin(), ptr.end());
		}

		return {ret, retmodels};

	} catch (std::exception& e) {
		std::cerr << "loadMap(): couldn't parse " << name
			<< ": " << e.what() << std::endl;

		// return empty map instead
		return {std::make_shared<gameObject>(), {}};
	}
}

gameObject::ptr grendx::loadMapCompiled(gameMain *game, std::string name) {
	auto [obj, models] = loadMapData(game, name);

	if (obj) {
		compileModels(models);
	}

	return obj;
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

static json objectJson(gameObject::ptr obj) {
	bool recurse = true;
	json ret = {
		{"type",     obj->typeString()},
		{"position", {obj->transform.position.x,
		              obj->transform.position.y,
		              obj->transform.position.z}},
		{"scale",    {obj->transform.scale.x,
		              obj->transform.scale.y,
		              obj->transform.scale.z}},
		{"rotation", {obj->transform.rotation.w,
		              obj->transform.rotation.x,
		              obj->transform.rotation.y,
		              obj->transform.rotation.z}},
	};

	// TODO: would make sense to avoid dynamic_pointer_cast here,
	//       not storing pointers anywhere
	if (obj->type == gameObject::objType::Import) {
		gameImport::ptr imported = std::dynamic_pointer_cast<gameImport>(obj);
		ret["sourceFile"] = imported->sourceFile;
		recurse = false;

	} else if (obj->type == gameObject::objType::Model) {
		gameModel::ptr model = std::dynamic_pointer_cast<gameModel>(obj);
		ret["sourceFile"] = model->sourceFile;
		recurse = false;

	} else if (obj->type == gameObject::objType::ReflectionProbe) {
		auto probe = std::dynamic_pointer_cast<gameReflectionProbe>(obj);
		auto bmin = probe->boundingBox.min;
		auto bmax = probe->boundingBox.max;

		ret["boundingBox"] = {
			{"min", {bmin[0], bmin[1], bmin[2]}},
			{"max", {bmax[0], bmax[1], bmax[2]}},
		};
		ret["is_static"] = probe->is_static;

	} else if (obj->type == gameObject::objType::IrradianceProbe) {
		auto probe = std::dynamic_pointer_cast<gameIrradianceProbe>(obj);
		auto bmin = probe->boundingBox.min;
		auto bmax = probe->boundingBox.max;

		ret["boundingBox"] = {
			{"min", {bmin[0], bmin[1], bmin[2]}},
			{"max", {bmax[0], bmax[1], bmax[2]}},
		};
		ret["is_static"] = probe->is_static;

	} else if (obj->type == gameObject::objType::Light) {
		gameLight::ptr light = std::dynamic_pointer_cast<gameLight>(obj);

		auto& d = light->diffuse;
		ret["diffuse"] = {d[0], d[1], d[2], d[3]};
		ret["intensity"] = light->intensity;
		ret["casts_shadows"] = light->casts_shadows;
		ret["is_static"] = light->is_static;

		if (light->lightType == gameLight::lightTypes::Point) {
			gameLightPoint::ptr p = std::dynamic_pointer_cast<gameLightPoint>(obj);
			ret["radius"] = p->radius;
		}

		else if (light->lightType == gameLight::lightTypes::Spot) {
			gameLightSpot::ptr p = std::dynamic_pointer_cast<gameLightSpot>(obj);
			ret["radius"] = p->radius;
			ret["angle"]  = p->angle;
		}

		else if (light->lightType == gameLight::lightTypes::Directional) {
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

void grendx::saveMap(gameMain *game,
                     gameObject::ptr root,
                     std::string name)
{
	std::ofstream foo(name);
	std::cerr << "saving map " << name << std::endl;
	std::cerr << "TODO: reimplement saving maps" << std::endl;

	if (!foo.good()) {
		std::cerr << "couldn't open save file" << name << std::endl;
		return;
	}

	json j = {{
		"map", {
			{"file", name},
		}}};

	j["root"] = objectJson(root);

	foo << j.dump(4) << std::endl;
}
