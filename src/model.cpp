#include <grend/model.hpp>
#include <grend/utility.hpp>

#include <vector>
#include <map>
#include <string>
#include <exception>
#include <iostream>
#include <fstream>
#include <sstream>

namespace grendx {

model::model(std::string filename) {
	load_object(filename);
}

void model::clear(void) {
	meshes.clear();
}

void model::gen_normals(void) {
	std::cerr << " > generating new normals... " << vertices.size() << std::endl;
	normals.resize(vertices.size(), glm::vec3(0));

	for (auto& thing : meshes) {
		for (unsigned i = 0; i < thing.second.faces.size(); i += 3) {
			GLushort elms[3] = {
				thing.second.faces[i],
				thing.second.faces[i+1],
				thing.second.faces[i+2]
			};

			glm::vec3 normal = glm::normalize(
					glm::cross(
						vertices[elms[1]] - vertices[elms[0]],
						vertices[elms[2]] - vertices[elms[0]]));

			normals[elms[0]] = normals[elms[1]] = normals[elms[2]] = normal;
		}
	}
}

void model::gen_texcoords(void) {
	std::cerr << " > generating new texcoords... " << vertices.size() << std::endl;
	texcoords.resize(2*vertices.size());

	for (unsigned i = 0; i < vertices.size(); i++) {
		glm::vec3& foo = vertices[i];
		texcoords[2*i]   = foo.x;
		texcoords[2*i+1] = foo.y;
	}
}

void model::gen_tangents(void) {
	std::cerr << " > generating tangents... " << vertices.size() << std::endl;
	// generate tangents for each triangle
	for (std::size_t i = 0; i < vertices.size(); i += 3) {
		glm::vec3& a = vertices[i];
		glm::vec3& b = vertices[i+1];
		glm::vec3& c = vertices[i+2];

		glm::vec2 auv = {texcoords[2*i], texcoords[2*i + 1]};
		glm::vec2 buv = {texcoords[2*(i+1)], texcoords[2*(i+1) + 1]};
		glm::vec2 cuv = {texcoords[2*(i+2)], texcoords[2*(i+2) + 1]};

		glm::vec3 e1 = b - a, e2 = c - a;
		glm::vec2 duv1 = buv - auv, duv2 = cuv - auv;

		glm::vec3 tangent, bitangent;

		float f = 1.f / (duv1.x * duv2.y - duv2.x * duv1.y);
		tangent.x = f * (duv2.y * e1.x + duv1.y * e2.x);
		tangent.y = f * (duv2.y * e1.y + duv1.y * e2.y);
		tangent.z = f * (duv2.y * e1.z + duv1.y * e2.z);
		tangent = glm::normalize(tangent);

		bitangent.x = f * (-duv2.x * e1.x + duv1.x * e2.x);
		bitangent.x = f * (-duv2.x * e1.y + duv1.x * e2.y);
		bitangent.x = f * (-duv2.x * e1.z + duv1.x * e2.z);
		bitangent = glm::normalize(bitangent);

		for (unsigned k = 0; k < 3; k++) {
			tangents.push_back(tangent);
			bitangents.push_back(bitangent);
		}
	}
}

static std::string base_dir(std::string filename) {
	std::size_t found = filename.rfind("/");

	if (found != std::string::npos) {
		return filename.substr(0, found) + "/";
	}

	return filename;
}

void model::load_object(std::string filename) {
	std::cerr << " > loading " << filename << std::endl;
	std::ifstream input(filename);
	std::string line;
	std::string current_mesh = "default";

	std::vector<glm::vec3> vertbuf = {};
	std::vector<glm::vec3> normbuf = {};
	std::vector<GLfloat> texbuf = {};

	if (!input.good()) {
		// TODO: exception
		std::cerr << " ! couldn't load object from " << filename << std::endl;
		return;
	}

	// in case we're reloading over a previously-loaded object
	clear();

	while (std::getline(input, line)) {
		auto statement = split_string(line);

		if (statement.size() < 2)
			continue;

		if (statement[0] == "o") {
			std::cerr << " > have submesh " << statement[1] << std::endl;
			current_mesh = statement[1];
		}

		else if (statement[0] == "mtllib") {
			std::string temp = base_dir(filename) + statement[1];
			std::cerr << " > using material " << temp << std::endl;
			load_materials(temp);
		}

		else if (statement[0] == "usemtl") {
			std::cerr << " > using material " << statement[1] << std::endl;
			meshes[current_mesh].material = statement[1];
		}

		else if (statement[0] == "v") {
			glm::vec3 v = glm::vec3(std::stof(statement[1]),
			                        std::stof(statement[2]),
			                        std::stof(statement[3]));
			vertbuf.push_back(v);
		}

		else if (statement[0] == "f") {
			std::size_t end = statement.size();
			unsigned elements_added = 0;

			// XXX: we should be checking for buffer ranges here
			auto load_face_tri = [&] (std::string& statement) {
				auto spec = split_string(statement, '/');
				unsigned vert_index = std::stoi(spec[0]) - 1;

				vertices.push_back(vertbuf[vert_index]);
				meshes[current_mesh].faces.push_back(vertices.size() - 1);

				if (spec.size() > 1 && !spec[1].empty()) {
					unsigned buf_index = 2*(std::stoi(spec[1]) - 1);
					texcoords.push_back(texbuf[buf_index]);
					texcoords.push_back(texbuf[buf_index + 1]);
				}

				if (spec.size() > 2 && !spec[2].empty()) {
					normals.push_back(normbuf[std::stoi(spec[2]) - 1]);
				}
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
			have_normals = true;
		}

		else if (statement[0] == "vt") {
			texbuf.push_back(std::stof(statement[1]));
			texbuf.push_back(std::stof(statement[2]));
			have_texcoords = true;
		}
	}

	// TODO: check that normals size == vertices size and fill in the difference
	if (!have_normals) {
		gen_normals();
	}

	if (!have_texcoords) {
		gen_texcoords();
	}

	gen_tangents();

	if (normals.size() != vertices.size()) {
		std::cerr << " ? mismatched normals and vertices: "
			<< normals.size() << ", "
			<< vertices.size()
			<< std::endl;
		// TODO: should handle this
	}

	if (texcoords.size()/2 != vertices.size()) {
		std::cerr << " ? mismatched texcoords and vertices: "
			<< texcoords.size() << ", "
			<< vertices.size()
			<< std::endl;
		// TODO: should handle this
	}

	if (tangents.size() != vertices.size()
	    || bitangents.size() != vertices.size())
	{
		std::cerr << " ? mismatched tangents and vertices: "
			<< tangents.size() << ", "
			<< vertices.size()
			<< std::endl;
	}
}

void model::load_materials(std::string filename) {
	std::ifstream input(filename);
	std::string current_material = "default";
	std::string line;

	if (!input.good()) {
		// TODO: exception
		std::cerr << "Warning: couldn't load material library from "
			<< filename << std::endl;
		return;
	}

	while (std::getline(input, line)) {
		auto statement = split_string(line);

		if (statement.size() < 2) {
			continue;
		}

		if (statement[0] == "newmtl") {
			std::cerr << "   - new material: " << statement[1] << std::endl;
			current_material = statement[1];
		}

		else if (statement[0] == "Ka") {
			materials[current_material].ambient = glm::vec4(std::stof(statement[1]),
			                                                std::stof(statement[2]),
			                                                std::stof(statement[3]), 1);
		}

		else if (statement[0] == "Kd") {
			materials[current_material].diffuse = glm::vec4(std::stof(statement[1]),
			                                                std::stof(statement[2]),
			                                                std::stof(statement[3]), 1);

		}

		else if (statement[0] == "Ks") {
			materials[current_material].specular = glm::vec4(std::stof(statement[1]),
			                                                 std::stof(statement[2]),
			                                                 std::stof(statement[3]), 1);
		}

		else if (statement[0] == "Ns") {
			materials[current_material].shininess = std::stof(statement[1]);
		}

		else if (statement[0] == "map_Kd") {
			materials[current_material].diffuse_map = base_dir(filename) + statement[1];
		}

		else if (statement[0] == "map_Ns") {
			// specular map
			materials[current_material].specular_map = base_dir(filename) + statement[1];
		}

		else if (statement[0] == "map_ao") {
			// ambient occlusion map (my own extension)
			materials[current_material].ambient_occ_map = base_dir(filename) + statement[1];
		}

		else if (statement[0] == "map_norm" || statement[0] == "norm") {
			// normal map (also non-standard)
			materials[current_material].normal_map = base_dir(filename) + statement[1];
		}

		else if (statement[0] == "map_bump") {
			// bump/height map
		}

		// TODO: other light maps, attributes
	}
}

// namespace grendx
}
