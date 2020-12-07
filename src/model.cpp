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

void gameModel::genNormals(void) {
	std::cerr << " > generating new normals... " << vertices.size() << std::endl;
	normals.resize(vertices.size(), glm::vec3(0));

	for (auto& [name, ptr] : nodes) {
		if (ptr->type != gameObject::objType::Mesh) {
			continue;
		}
		gameMesh::ptr mesh = std::dynamic_pointer_cast<gameMesh>(ptr);

		for (unsigned i = 0; i < mesh->faces.size(); i += 3) {
			// TODO: bounds check
			GLuint elms[3] = {
				mesh->faces[i],
				mesh->faces[i+1],
				mesh->faces[i+2]
			};

			glm::vec3 normal = glm::normalize(
					glm::cross(
						vertices[elms[1]] - vertices[elms[0]],
						vertices[elms[2]] - vertices[elms[0]]));

			normals[elms[0]] = normals[elms[1]] = normals[elms[2]] = normal;
		}
	}
}

void gameModel::genTexcoords(void) {
	std::cerr << " > generating new texcoords... " << vertices.size() << std::endl;
	texcoords.resize(vertices.size());

	for (auto& [name, ptr] : nodes) {
		if (ptr->type != gameObject::objType::Mesh) {
			continue;
		}
		gameMesh::ptr mesh = std::dynamic_pointer_cast<gameMesh>(ptr);

		for (unsigned i = 0; i < mesh->faces.size(); i++) {
			// TODO: bounds check
			GLuint elm = mesh->faces[i];

			glm::vec3& foo = vertices[elm];
			texcoords[elm] = {foo.x, foo.y};
		}
	}
}

void gameModel::genAABBs(void) {
	std::cerr << " > generating axis-aligned bounding boxes..." << std::endl;

	for (auto& [name, ptr] : nodes) {
		if (ptr->type != gameObject::objType::Mesh) {
			continue;
		}
		gameMesh::ptr mesh = std::dynamic_pointer_cast<gameMesh>(ptr);

		if (mesh->faces.size() == 0) {
			std::cerr << " > have face with no vertices...?" << std::endl;
			continue;
		}

		// set base value for min/max, needs to be something in the mesh
		// so first element will do
		mesh->boundingBox.min = mesh->boundingBox.max
			= vertices[mesh->faces[0]];

		for (unsigned i = 0; i < mesh->faces.size(); i++) {
			// TODO: bounds check
			GLuint elm = mesh->faces[i];
			glm::vec3& foo = vertices[elm];

			// TODO: check, think this is right
			mesh->boundingBox.min = min(mesh->boundingBox.min, foo);
			mesh->boundingBox.max = max(mesh->boundingBox.max, foo);
		}
	}
}

void gameModel::genTangents(void) {
	std::cerr << " > generating tangents... " << vertices.size() << std::endl;
	// allocate a little bit extra to make sure we don't overrun the buffer...
	// XXX:
	// TODO: just realized, this should be working with faces, not vertices
	//       so fix this after gltf stuff is in a workable state
	//unsigned mod = 3 - vertices.size()%3;
	unsigned mod = 3;

	tangents.resize(vertices.size(), glm::vec3(0));

	// generate tangents for each triangle
	for (auto& [name, ptr] : nodes) {
		if (ptr->type != gameObject::objType::Mesh) {
			continue;
		}
		gameMesh::ptr mesh = std::dynamic_pointer_cast<gameMesh>(ptr);

		for (std::size_t i = 0; i+2 < mesh->faces.size(); i += 3) {
			// TODO: bounds check
			GLuint elms[3] = {mesh->faces[i], mesh->faces[i+1], mesh->faces[i+2]};

			glm::vec3& a = vertices[elms[0]];
			glm::vec3& b = vertices[elms[1]];
			glm::vec3& c = vertices[elms[2]];

			glm::vec2 auv = texcoords[elms[0]];
			glm::vec2 buv = texcoords[elms[1]];
			glm::vec2 cuv = texcoords[elms[2]];

			glm::vec3 e1 = b - a, e2 = c - a;
			glm::vec2 duv1 = buv - auv, duv2 = cuv - auv;

			glm::vec3 tangent;

			float f = 1.f / (duv1.x * duv2.y - duv2.x * duv1.y);
			tangent.x = f * (duv2.y * e1.x + duv1.y * e2.x);
			tangent.y = f * (duv2.y * e1.y + duv1.y * e2.y);
			tangent.z = f * (duv2.y * e1.z + duv1.y * e2.z);

			tangents[elms[0]] = tangents[elms[1]] = tangents[elms[2]]
				= glm::normalize(tangent);
		}
	}
}

// namespace grendx
}
