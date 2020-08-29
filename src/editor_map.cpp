#include <grend/game_editor.hpp>
#include <grend/utility.hpp>
#include <iostream>
#include <fstream>

#define IMGUI_IMPL_OPENGL_LOADER_GLEW
#include <imgui/imgui.h>
#include <imgui/examples/imgui_impl_sdl.h>
#include <imgui/examples/imgui_impl_opengl3.h>

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

static model_map xxx_load_model(std::string filename, std::string objName) {
	auto ext = filename_extension(filename);
	model_map models;

	if (ext == ".obj") {
		auto model = load_object(filename);
		models[objName] = model;
		compile_model(objName, model);

	} else if (ext == ".gltf") {
		models = load_gltf_models(filename);
		compile_models(models);
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

			model_map& models = sources[source];

			return (models.find(name) != models.end())
				? models[name]
				: nullptr;
		}

		std::map<std::string, model_map> sources;
};

gameObject::ptr loadNodes(modelCache& cache, std::string name, json jay) {
	gameObject::ptr ret = nullptr;
	std::vector<std::string> modelFiles;
	bool recurse = true;

	if (jay["type"] == "Model") {
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

	// TODO: other light types

	} else if (jay["type"] == "Reflection probe"){
		auto probe = std::make_shared<gameReflectionProbe>();
		probe->is_static = jay["is_static"];
		ret = std::dynamic_pointer_cast<gameObject>(probe);

	} else {
		ret = std::make_shared<gameObject>();
	}

	auto& pos   = jay["position"];
	auto& scale = jay["scale"];
	auto& rot   = jay["rotation"];

	std::cerr << jay.dump(3) << std::endl;

	ret->position = glm::vec3(pos[0], pos[1], pos[2]);
	ret->scale    = glm::vec3(scale[0], scale[1], scale[2]);
	ret->rotation = glm::quat(rot[0], rot[1], rot[2], rot[3]);

	if (recurse && !jay["nodes"].is_null()) {
		for (auto& [name, ptr] : jay["nodes"].items()) {
			if (!ptr.is_null()) {
				setNode(name, ret, loadNodes(cache, name, ptr));
			}
		}
	}

	return ret;
}

gameObject::ptr grendx::loadMap(gameMain *game, std::string name) {
	std::ifstream foo(name);
	std::cerr << "loading map " << name << std::endl;

	if (!foo.good()) {
		std::cerr << "couldn't open save file" << name << std::endl;
		return gameObject::ptr(new gameObject());
	}

	json j;
	model_map models;

	try {
		gameObject::ptr ret = nullptr;
		foo >> j;

		// XXX: again TODO
		modelCache cache;
		ret = loadNodes(cache, "", j["root"]);

		// assume there were new loaded models
		bind_cooked_meshes();
		return ret;

	} catch (std::exception& e) {
		std::cerr << "loadMap(): couldn't parse " << name
			<< ": " << e.what() << std::endl;

		// return empty map instead
		return gameObject::ptr(new gameObject());
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

static json objectJson(gameObject::ptr obj) {
	json ret = {
		{"type",     obj->typeString()},
		{"position", {obj->position.x, obj->position.y, obj->position.z}},
		{"scale",    {obj->scale.x, obj->scale.y, obj->scale.z}},
		{"rotation", {obj->rotation.w, obj->rotation.x,
		              obj->rotation.y, obj->rotation.z}},
	};

	// TODO: would make sense to avoid dynamic_pointer_cast here,
	//       not storing pointers anywhere
	if (obj->type == gameObject::objType::Model) {
		gameModel::ptr model = std::dynamic_pointer_cast<gameModel>(obj);
		ret["sourceFile"] = model->sourceFile;

	} else if (obj->type == gameObject::objType::ReflectionProbe) {
		auto probe = std::dynamic_pointer_cast<gameReflectionProbe>(obj);
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

		// TODO: other lights
	}

	json nodes = {};
	for (auto& [name, ptr] : obj->nodes) {
		nodes[name] = objectJson(ptr);
	}

	ret["nodes"] = nodes;
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
