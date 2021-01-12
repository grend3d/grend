#include <grend/gameModel.hpp>
#include <grend/utility.hpp>

#include <stb/stb_image.h>
#include <stb/stb_image_write.h>

#include <vector>
#include <map>
#include <string>
#include <exception>
#include <iostream>
#include <fstream>
#include <sstream>
#include <optional>

#include <stdint.h>

namespace grendx {

static std::string base_dir(std::string filename) {
	std::size_t found = filename.rfind("/");

	if (found != std::string::npos) {
		return filename.substr(0, found) + "/";
	}

	return filename;
}

gameModel::ptr load_object(std::string filename) {
	std::cerr << " > loading " << filename << std::endl;
	std::ifstream input(filename);
	std::string line;
	std::string mesh_name = "default";
	std::string mesh_material = "(null)";
	std::string current_mesh_name = mesh_name + ":" + mesh_material;
	gameMesh::ptr current_mesh = nullptr;

	std::vector<glm::vec3> vertbuf = {};
	std::vector<glm::vec3> normbuf = {};
	std::vector<glm::vec2> texbuf = {};
	std::map<std::string, material::ptr> materials;

	if (!input.good()) {
		// TODO: exception
		std::cerr << " ! couldn't load object from " << filename << std::endl;
		return nullptr;
	}

	gameModel::ptr ret = gameModel::ptr(new gameModel());
	ret->sourceFile = filename;

	// TODO: split this into a seperate parse function
	while (std::getline(input, line)) {
		auto statement = split_string(line);

		if (statement.size() < 2)
			continue;

		if (statement[0] == "o") {
			std::cerr << " > have submesh " << statement[1] << std::endl;
			// TODO: should current_mesh just be defined at the top
			//       of this loop?
			mesh_name = statement[1];
			current_mesh_name = mesh_name + ":(null)";
			current_mesh = gameMesh::ptr(new gameMesh());
			setNode(current_mesh_name, ret, current_mesh);
		}

		else if (statement[0] == "mtllib") {
			std::string temp = base_dir(filename) + statement[1];
			std::cerr << " > using material " << temp << std::endl;
			auto mats = load_materials(ret, temp);
			materials.insert(mats.begin(), mats.end());
		}

		else if (statement[0] == "usemtl") {
			std::cerr << " > using material " << statement[1] << std::endl;
			current_mesh = gameMesh::ptr(new gameMesh());
			//current_mesh->material = statement[1];
			// TODO: check that material exists
			current_mesh->meshMaterial = materials[statement[1]];
			current_mesh_name = mesh_name + ":" + statement[1];

			setNode(current_mesh_name, ret, current_mesh);
		}

		else if (statement[0] == "v") {
			glm::vec3 v = glm::vec3(std::stof(statement[1]),
			                        std::stof(statement[2]),
			                        std::stof(statement[3]));
			vertbuf.push_back(v);
		}

		else if (statement[0] == "f") {
			std::size_t end = statement.size();

			// TODO: XXX: we should be checking for buffer ranges here
			auto load_face_tri = [&] (std::string& statement) {
				glm::vec3 position(0);
				glm::vec3 normal(0);
				glm::vec2 texcoord(0);

				auto spec = split_string(statement, '/');
				unsigned vert_index = std::stoi(spec[0]) - 1;

				position = vertbuf[vert_index];
				//ret->vertices.push_back(vertbuf[vert_index]);

				if (ret->haveTexcoords && spec.size() > 1 && !spec[1].empty()) {
					unsigned buf_index = std::stoi(spec[1]) - 1;
					//ret->texcoords.push_back(texbuf[buf_index]);
					texcoord = texbuf[buf_index];
				}

				if (ret->haveNormals && spec.size() > 2 && !spec[2].empty()) {
					//ret->normals.push_back(normbuf[std::stoi(spec[2]) - 1]);
					normal = normbuf[std::stoi(spec[2]) - 1];
				}

				ret->vertices.push_back((gameModel::vertex) {
					.position = position,
					.normal   = normal,
					.uv       = texcoord,
				});

				current_mesh->faces.push_back(ret->vertices.size() - 1);
			};

			for (std::size_t cur = 1; cur + 2 < end; cur++) {
				load_face_tri(statement[1]);

				for (unsigned k = 1; k < 3; k++) {
					load_face_tri(statement[cur + k]);
				}
			}
		}

		else if (statement[0] == "vn") {
			glm::vec3 v = glm::vec3(std::stof(statement[1]),
			                        std::stof(statement[2]),
			                        std::stof(statement[3]));
			//normals.push_back(glm::normalize(v));
			normbuf.push_back(v);
			ret->haveNormals = true;
		}

		else if (statement[0] == "vt") {
			texbuf.push_back({std::stof(statement[1]), std::stof(statement[2])});
			ret->haveTexcoords = true;
		}

	}

	for (const auto& [name, ptr] : ret->nodes) {
		std::cerr << " > > have mesh node " << name << std::endl; 
	}

	// TODO: check that normals size == vertices size and fill in the difference
	if (!ret->haveNormals) {
		ret->genNormals();
	}

	if (!ret->haveTexcoords) {
		ret->genTexcoords();
	}

	ret->genTangents();
	ret->genAABBs();

	return ret;
}

materialTexture::materialTexture(std::string filename) {
	load_texture(filename);
}

void materialTexture::load_texture(std::string filename) {
	std::cerr << "loading " << filename << std::endl;
	uint8_t *datas = stbi_load(filename.c_str(), &width,
	                           &height, &channels, 0);

	if (!datas) {
		throw std::logic_error("Couldn't load material texture " + filename);
	}

	size_t imgsize = width * height * channels;
	//pixels.insert(pixels.end(), datas, datas + imgsize);
	for (unsigned i = 0; i < imgsize; i++) {
		pixels.push_back(datas[i]);
	}

	stbi_image_free(datas);
}

std::map<std::string, material::ptr>
load_materials(gameModel::ptr model, std::string filename) {
	std::map<std::string, material::ptr> ret;
	std::ifstream input(filename);
	std::string current_material = "default";
	std::string line;

	if (!input.good()) {
		// TODO: exception
		std::cerr << "Warning: couldn't load material library from "
			<< filename << std::endl;
		return ret;
	}

	stbi_set_flip_vertically_on_load(true);

	while (std::getline(input, line)) {
		auto statement = split_string(line);

		if (statement.size() < 2) {
			continue;
		}

		if (statement[0] == "newmtl") {
			std::cerr << "   - new material: " << statement[1] << std::endl;
			current_material = statement[1];
			ret[current_material] = std::make_shared<material>();
		}

		else if (statement[0] == "Ka") {
			ret[current_material]->factors.ambient =
				glm::vec4(std::stof(statement[1]),
			              std::stof(statement[2]),
			              std::stof(statement[3]), 1);
		}

		else if (statement[0] == "Kd") {
			ret[current_material]->factors.diffuse =
				glm::vec4(std::stof(statement[1]),
			              std::stof(statement[2]),
			              std::stof(statement[3]), 1);
		}

		else if (statement[0] == "Ks") {
			ret[current_material]->factors.specular =
				glm::vec4(std::stof(statement[1]),
			             std::stof(statement[2]),
			             std::stof(statement[3]), 1);
		}

		else if (statement[0] == "d") {
			ret[current_material]->factors.opacity
				= std::stof(statement[1]);
		}

		else if (statement[0] == "Ns") {
			ret[current_material]->factors.roughness =
				1.f - std::stof(statement[1])/1000.f;
		}

		else if (statement[0] == "illum") {
			unsigned illumination_model = std::stoi(statement[1]);

			switch (illumination_model) {
				case 4:
				case 6:
				case 7:
				case 9:
					ret[current_material]->factors.blend
						= material::blend_mode::Blend;
					break;

				default:
					ret[current_material]->factors.blend
						= material::blend_mode::Opaque;
					break;
			}
		}

		else if (statement[0] == "map_Kd") {
			ret[current_material]->maps.diffuse =
				std::make_shared<materialTexture>(base_dir(filename) + statement[1]);
		}

		else if (statement[0] == "map_Ns") {
			// specular map
			ret[current_material]->maps.metalRoughness =
				std::make_shared<materialTexture>(base_dir(filename) + statement[1]);
		}

		else if (statement[0] == "map_ao") {
			// ambient occlusion map (my own extension)
			ret[current_material]->maps.ambientOcclusion =
				std::make_shared<materialTexture>(base_dir(filename) + statement[1]);
		}

		else if (statement[0] == "map_norm" || statement[0] == "norm") {
			// normal map (also non-standard)
			ret[current_material]->maps.normal =
				std::make_shared<materialTexture>(base_dir(filename) + statement[1]);
		}

		else if (statement[0] == "map_bump") {
			// bump/height map
		}

		/*
		else if (statement[0] == "Ke" || statement[0] == "map_Ke") {
			model->materials[current_material].emissiveMap.
				load_texture(base_dir(filename) + statement[1]);
		}
		*/

		// TODO: other light maps, attributes
	}

	stbi_set_flip_vertically_on_load(false);
	return ret;
}

// namespace grendx
}
