#include <grend/sceneModel.hpp>
#include <grend/utility.hpp>
#include <grend/logger.hpp>
#include <grend/ecs/bufferComponent.hpp>

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

// XXX: "key functions"
sceneMesh::~sceneMesh() {};
sceneModel::~sceneModel() {};

void sceneModel::genNormals(void) {
	for (auto link : nodes()) {
		auto ptr = link->getRef();

		if (ptr->type != sceneNode::objType::Mesh) {
			continue;
		}

		sceneMesh::ptr mesh = ref_cast<sceneMesh>(ptr);
		auto vertBuf = this->get<ecs::bufferComponent<sceneModel::vertex>>();
		auto faceBuf = mesh->get<ecs::bufferComponent<sceneMesh::faceType>>();

		if (!vertBuf || !faceBuf)
			continue;

		auto& faces = faceBuf->data;
		auto& verts = vertBuf->data;

		for (unsigned i = 0; i < faces.size(); i += 3) {
			// TODO: bounds check
			GLuint elms[3] = {
				faces[i],
				faces[i+1],
				faces[i+2]
			};

			if (elms[0] >= verts.size()
			 || elms[1] >= verts.size()
			 || elms[2] >= verts.size())
			{
				LogError(" > invalid face index! (genNormals())");
				break;
			}

			glm::vec3 normal = glm::normalize(
				glm::cross(
					verts[elms[1]].position - verts[elms[0]].position,
					verts[elms[2]].position - verts[elms[0]].position));

			verts[elms[0]].normal
				= verts[elms[1]].normal
				= verts[elms[2]].normal = normal;
		}
	}
}

void sceneModel::genTexcoords(void) {
	for (auto link : nodes()) {
		auto ptr = link->getRef();

		if (ptr->type != sceneNode::objType::Mesh) {
			continue;
		}
		sceneMesh::ptr mesh = ref_cast<sceneMesh>(ptr);

		auto vertBuf = this->get<ecs::bufferComponent<sceneModel::vertex>>();
		auto faceBuf = mesh->get<ecs::bufferComponent<sceneMesh::faceType>>();

		if (!vertBuf || !faceBuf)
			continue;

		auto& faces = faceBuf->data;
		auto& verts = vertBuf->data;

		for (unsigned i = 0; i < faces.size(); i++) {
			// TODO: bounds check
			GLuint elm = faces[i];

			if (elm >= verts.size()) {
				LogError(" > invalid face index! (genTexcoords())");
				continue;
			}

			glm::vec3& foo = verts[elm].position;
			verts[elm].uv = {foo.x, foo.y};
		}
	}
}

void sceneModel::genAABBs(void) {
	for (auto link : nodes()) {
		auto ptr = link->getRef();

		if (ptr->type != sceneNode::objType::Mesh) {
			continue;
		}
		sceneMesh::ptr mesh = ref_cast<sceneMesh>(ptr);

		auto vertBuf = this->get<ecs::bufferComponent<sceneModel::vertex>>();
		auto faceBuf = mesh->get<ecs::bufferComponent<sceneMesh::faceType>>();

		if (!vertBuf || !faceBuf)
			continue;

		auto& faces = faceBuf->data;
		auto& verts = vertBuf->data;

		if (faces.size() == 0) {
			LogError(" > have face with no vertices...?");
			continue;
		}

		// set base value for min/max, needs to be something in the mesh
		// so first element will do
		mesh->boundingBox.min = mesh->boundingBox.max
			= verts[faces[0]].position;

		for (unsigned i = 0; i < faces.size(); i++) {
			// TODO: bounds check
			GLuint elm = faces[i];

			if (elm >= verts.size()) {
				LogError(" > invalid face index! (genAABBs())");
				continue;
			}

			glm::vec3& foo = verts[elm].position;
			mesh->boundingBox.min = min(mesh->boundingBox.min, foo);
			mesh->boundingBox.max = max(mesh->boundingBox.max, foo);
		}

		mesh->boundingSphere = AABBToBSphere(mesh->boundingBox);
	}
}

void sceneModel::genTangents(void) {
	// generate tangents for each triangle
	for (auto link : nodes()) {
		auto ptr = link->getRef();

		if (ptr->type != sceneNode::objType::Mesh) {
			continue;
		}
		sceneMesh::ptr mesh = ref_cast<sceneMesh>(ptr);

		auto vertBuf = this->get<ecs::bufferComponent<sceneModel::vertex>>();
		auto faceBuf = mesh->get<ecs::bufferComponent<sceneMesh::faceType>>();

		if (!vertBuf || !faceBuf)
			continue;

		auto& faces = faceBuf->data;
		auto& verts = vertBuf->data;

		for (std::size_t i = 0; i+2 < faces.size(); i += 3) {
			// TODO: bounds check
			GLuint elms[3] = {faces[i], faces[i+1], faces[i+2]};

			if (elms[0] >= verts.size()
			 || elms[1] >= verts.size()
			 || elms[2] >= verts.size())
			{
				LogError(" > invalid face index! (genTangents())");
				break;
			}

			glm::vec3& a = verts[elms[0]].position;
			glm::vec3& b = verts[elms[1]].position;
			glm::vec3& c = verts[elms[2]].position;

			glm::vec2 auv = verts[elms[0]].uv;
			glm::vec2 buv = verts[elms[1]].uv;
			glm::vec2 cuv = verts[elms[2]].uv;

			glm::vec3 e1 = b - a, e2 = c - a;
			glm::vec2 duv1 = buv - auv, duv2 = cuv - auv;

			glm::vec3 tangent;

			float f = 1.f / (duv1.x * duv2.y - duv2.x * duv1.y);
			tangent.x = f * (duv2.y * e1.x + duv1.y * e2.x);
			tangent.y = f * (duv2.y * e1.y + duv1.y * e2.y);
			tangent.z = f * (duv2.y * e1.z + duv1.y * e2.z);

			verts[elms[0]].tangent
				= verts[elms[1]].tangent
				= verts[elms[2]].tangent
				= glm::vec4(glm::normalize(tangent), 1.0);
		}
	}
}

// namespace grendx
}
