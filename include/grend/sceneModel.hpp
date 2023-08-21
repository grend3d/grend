#pragma once

#include <grend/material.hpp>
#include <grend/sceneNode.hpp>
#include <grend/glmIncludes.hpp>
#include <grend/openglIncludes.hpp>
#include <grend/boundingBox.hpp>
#include <string>
#include <vector>
#include <map>
#include <memory>

#include <grend/ecs/serializeBuilder.hpp>
#include <grend/ecs/glmSerializers.hpp>

#include <stdint.h>

namespace grendx {

// defined in glManager.hpp
class compiledMesh;
class compiledModel;

class sceneMesh : public sceneNode {
	public:
		typedef ecs::ref<sceneMesh> ptr;
		typedef ecs::ref<sceneMesh> weakptr;

		typedef GLuint faceType;

		sceneMesh(ecs::regArgs t)
			: sceneNode(ecs::doRegister(this, t), objType::Mesh)
		{ };

		virtual ~sceneMesh();

		static nlohmann::json serializer(component *comp);
		static void deserializer(component *comp, nlohmann::json j);

		// TODO: don't store reference to compiled data here
		std::shared_ptr<compiledMesh> comped_mesh;
		bool compiled = false;

		struct AABB    boundingBox;
		struct BSphere boundingSphere;
};

// used for joint indices
typedef glm::vec<4, uint16_t, glm::defaultp> usvec4;
typedef glm::vec<4, uint8_t,  glm::defaultp> ubvec4;

class sceneModel : public sceneNode {
	public:
		typedef ecs::ref<sceneModel> ptr;
		typedef ecs::ref<sceneModel> weakptr;

		// TODO: move these out of sceneModel
		struct vertex {
			glm::vec3 position;
			glm::vec3 normal;
			glm::vec4 tangent;
			glm::vec3 color;
			glm::vec2 uv;
			glm::vec2 lightmap;

			static void serializeBytes(struct vertex *v, uint8_t *buf, size_t offset) {
				ecs::serializeBuilder(buf, offset)
					<< v->position
					<< v->normal
					<< v->tangent
					<< v->color
					<< v->uv
					<< v->lightmap;
			}

			static void deserializeBytes(struct vertex *v, uint8_t *buf, size_t offset) {
				ecs::deserializeBuilder(buf, offset)
					<< v->position
					<< v->normal
					<< v->tangent
					<< v->color
					<< v->uv
					<< v->lightmap;
			}

			static size_t serializedByteSize(void) {
				static struct vertex v;

				auto temp = ecs::serializeSizeBuilder()
					<< v.position
					<< v.normal
					<< v.tangent
					<< v.color
					<< v.uv
					<< v.lightmap;

				return temp.size;
			}
		};

		struct jointWeights {
			glm::vec4 joints;  // joints that affect the vertex
			glm::vec4 weights; // how much the joint affects the vertex
							   //
			static void serializeBytes(jointWeights *v, uint8_t *buf, size_t offset) {}
			static void deserializeBytes(jointWeights *v, uint8_t *buf, size_t offset) {}
			static size_t serializedByteSize(void) { return sizeof(struct jointWeights); }
		};

		sceneModel(ecs::regArgs t)
			: sceneNode(ecs::doRegister(this, t), objType::Model)
		{ };

		virtual ~sceneModel();

		void genInfo(void);
		void genNormals(void);
		void genTexcoords(void);
		void genTangents(void);
		void genAABBs(void);

		static nlohmann::json serializer(component *comp);
		static void deserializer(component *comp, nlohmann::json j);

		// TODO: don't store reference to compiled model here, just a container for data
		bool compiled = false;
		std::shared_ptr<compiledModel> comped_model;

		// TODO: vertex data can be freed after compiling, can keep a
		//       reference to the compiled model, which would also make it
		//       much easier to free models from the GPU

		// used to determine if normals, etc need to be generated
		bool haveNormals = false;
		bool haveColors = false;
		bool haveTangents = false;
		bool haveTexcoords = false;
		bool haveLightmap = false;
		bool haveJoints = false;
		bool haveAABB = false;
};

typedef std::map<std::string, sceneMesh::ptr> meshMap;
typedef std::map<std::string, sceneModel::ptr> modelMap;

sceneModel::ptr load_object(std::string filename);

modelMap load_gltf_models(std::string filename);
std::pair<sceneImport::ptr, modelMap> load_gltf_scene(std::string filename);
// TODO: load scene

// namespace grendx
}

// handle forward declaration of compiledModel
#include <grend/glManager.hpp>
