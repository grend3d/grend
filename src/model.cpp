#include <grend/model.hpp>

#include <vector>
#include <map>
#include <string>
#include <exception>
#include <iostream>
#include <fstream>
#include <sstream>

namespace grendx {

static std::vector<std::string> split_string(std::string s, char delim=' ') {
	std::vector<std::string> ret;
	std::size_t pos = std::string::npos, last = 0;

	for (pos = s.find(delim); pos != std::string::npos; pos = s.find(delim, pos + 1)) {
		ret.push_back(s.substr(last, pos - last));
		last = pos + 1;
	}

	ret.push_back(s.substr(last));
	return ret;
}

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

static std::string base_dir(std::string filename) {
	std::size_t found = filename.rfind("/");

	if (found != std::string::npos) {
		return filename.substr(0, found) + "/";
	}

	return filename;
}

void model::load_object(std::string filename) {
	std::ifstream input(filename);
	std::string line;
	std::string current_mesh = "default";

	std::vector<glm::vec3> vertbuf;
	std::vector<glm::vec3> normbuf;
	std::vector<GLfloat> texbuf;

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

		// TODO: other light maps, attributes
	}
}

// namespace grendx
}
