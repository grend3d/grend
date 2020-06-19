#include <grend/game_editor.hpp>
#include <grend/utility.hpp>
#include <iostream>
#include <fstream>

#define IMGUI_IMPL_OPENGL_LOADER_GLEW
#include <imgui/imgui.h>
#include <imgui/examples/imgui_impl_sdl.h>
#include <imgui/examples/imgui_impl_opengl3.h>

using namespace grendx;

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

void game_editor::load_map(renderer *rend, std::string name) {
	std::ifstream foo(name);
	std::cerr << "loading map " << name << std::endl;
	bool imported_models = false;

	if (!foo.good()) {
		std::cerr << "couldn't open save file" << name << std::endl;
		return;
	}

	// TODO: split this into smaller functions
	std::string line;
	while (std::getline(foo, line)) {
		auto statement = split_string(line, '\t');
		if (line[0] == '#' || line[0] == '\n') {
			continue;
		}

		if (statement[0] == "objfile" && statement.size() == 2) {
			load_model(rend, statement[1]);
			imported_models = true;
		}

		if (statement[0] == "scene" && statement.size() == 2) {
			load_model(rend, statement[1]);
			imported_models = true;
		}

		if (statement[0] == "entity" && statement.size() >= 6) {
			// TODO: size check
			auto matvec = strvec_to_float(split_string(statement[4], ','));

			editor_entry v;
			v.name = statement[1];
			v.classname = statement[2];
			v.position = parse_vec<glm::vec3>(statement[3]);
			v.inverted = std::stoi(statement[5]);

			for (unsigned i = 0; i < 16; i++) {
				v.transform[i/4][i%4] = matvec[i];
			}

			dynamic_models.push_back(v);
			std::cerr << "# loaded a " << v.name << std::endl;
		}

		if (statement[0] == "reflection_probe" && statement.size() == 3) {
			// TODO: size check
			glm::vec3 pos = parse_vec<glm::vec3>(statement[1]);

			rend->add_reflection_probe((struct reflection_probe) {
				.position = pos,
				.changed = true,
			});
		}

		if (statement[0] == "point_light" && statement.size() == 7) {
			glm::vec3 pos = parse_vec<glm::vec3>(statement[1]);
			glm::vec4 diffuse = parse_vec<glm::vec4>(statement[2]);
			float radius = std::stof(statement[3]);
			float intensity = std::stof(statement[4]);
			bool casts_shadows = std::stoi(statement[5]);
			bool static_shadows = std::stoi(statement[6]);

			uint32_t nlit = rend->add_light((struct point_light){
				.position = pos,
				.diffuse = diffuse,
				.radius = radius,
				.intensity = intensity,
				.casts_shadows = casts_shadows,
				.static_shadows = static_shadows,
			});

			edit_lights.point.push_back(nlit);
		}

		if (statement[0] == "spot_light" && statement.size() == 9) {
			glm::vec3 pos = parse_vec<glm::vec3>(statement[1]);
			glm::vec4 diffuse = parse_vec<glm::vec4>(statement[2]);
			glm::vec3 direction = parse_vec<glm::vec3>(statement[3]);
			float radius = std::stof(statement[4]);
			float intensity = std::stof(statement[5]);
			float angle = std::stof(statement[6]);
			bool casts_shadows = std::stoi(statement[7]);
			bool static_shadows = std::stoi(statement[8]);

			uint32_t nlit = rend->add_light((struct spot_light){
				.position = pos,
				.diffuse = diffuse,
				.direction = direction,
				.radius = radius,
				.intensity = intensity,
				.angle = angle,
				.casts_shadows = casts_shadows,
				.static_shadows = static_shadows,
			});

			edit_lights.spot.push_back(nlit);
		}

		if (statement[0] == "directional_light" && statement.size() == 7) {
			glm::vec3 pos = parse_vec<glm::vec3>(statement[1]);
			glm::vec4 diffuse = parse_vec<glm::vec4>(statement[2]);
			glm::vec3 direction = parse_vec<glm::vec3>(statement[3]);
			float intensity = std::stof(statement[4]);
			bool casts_shadows = std::stoi(statement[5]);
			bool static_shadows = std::stoi(statement[6]);

			uint32_t nlit = rend->add_light((struct directional_light){
				.position = pos,
				.diffuse = diffuse,
				.direction = direction,
				.intensity = intensity,
				.casts_shadows = casts_shadows,
				.static_shadows = static_shadows,
			});

			edit_lights.spot.push_back(nlit);
		}
	}

	if (imported_models) {
		// TODO: XXX: no need for const
		auto& glman = (gl_manager&)rend->get_glman();
		glman.bind_cooked_meshes();
		update_models(rend);
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

void game_editor::save_map(renderer *rend, std::string name) {
	std::ofstream foo(name);
	std::cerr << "saving map " << name << std::endl;

	if (!foo.good()) {
		std::cerr << "couldn't open save file" << name << std::endl;
		return;
	}

	foo << "### test scene save file" << std::endl;

	for (auto& path : editor_model_files) {
		foo << "objfile\t" << path << std::endl;
	}

	for (auto& path : editor_scene_files) {
		foo << "scene\t" << path << std::endl;
	}

	for (auto& v : dynamic_models) {
		foo << "entity\t" << v.name << "\t" << v.classname << "\t"
			<< format_vec(v.position) << "\t";

		foo << format_mat(v.transform) << "\t";
		foo << v.inverted;
		foo << std::endl;
	}

	for (auto& [id, p] : rend->ref_probes) {
		foo << "reflection_probe\t"
		    << format_vec(p.position) << "\t"
			<< p.is_static
		    << std::endl;
	}

	for (auto& id : edit_lights.point) {
		if (rend->point_lights.find(id) != rend->point_lights.end()) {
			auto lit = rend->get_point_light(id);

			foo << "point_light\t"
			    << format_vec(lit.position) << "\t"
			    << format_vec(lit.diffuse) << "\t"
			    << lit.radius << "\t"
			    << lit.intensity << "\t"
			    << lit.casts_shadows << "\t"
			    << lit.static_shadows
			    << std::endl;
		}
	}

	for (auto& id : edit_lights.spot) {
		if (rend->spot_lights.find(id) != rend->spot_lights.end()) {
			auto lit = rend->get_spot_light(id);

			foo << "spot_light\t"
			    << format_vec(lit.position) << "\t"
			    << format_vec(lit.diffuse) << "\t"
			    << format_vec(lit.direction) << "\t"
			    << lit.radius << "\t"
				<< lit.intensity << "\t"
				<< lit.angle << "\t"
			    << lit.casts_shadows << "\t"
			    << lit.static_shadows
			    << std::endl;
		}
	}

	for (auto& id : edit_lights.directional) {
		if (rend->directional_lights.find(id)
		    != rend->directional_lights.end())
		{
			auto lit = rend->get_directional_light(id);

			foo << "directional_light\t"
			    << format_vec(lit.position) << "\t"
			    << format_vec(lit.diffuse) << "\t"
			    << format_vec(lit.direction) << "\t"
			    << lit.intensity << "\t"
			    << lit.casts_shadows << "\t"
			    << lit.static_shadows
			    << std::endl;
		}
	}
}
