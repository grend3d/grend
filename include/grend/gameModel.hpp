#pragma once

#include <grend/material.hpp>
#include <grend/gameObject.hpp>
#include <grend/glmIncludes.hpp>
#include <grend/openglIncludes.hpp>
#include <grend/boundingBox.hpp>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <tinygltf/tiny_gltf.h>

#include <stdint.h>

namespace grendx {

// defined in glManager.hpp
class compiledMesh;
class compiledModel;

class gameMesh : public gameObject {
	public:
		typedef std::shared_ptr<gameMesh> ptr;
		typedef std::weak_ptr<gameMesh> weakptr;

		gameMesh() : gameObject(objType::Mesh) {};

		virtual std::string typeString(void) {
			return "Mesh";
		}

		std::shared_ptr<compiledMesh> comped_mesh;
		bool compiled = false;

		std::string meshName = "unit_cube:default";
		//std::string material = "(null)";
		material::ptr meshMaterial;
		std::vector<GLuint> faces;
		struct AABB boundingBox;
};

// used for joint indices
typedef glm::vec<4, uint16_t, glm::defaultp> usvec4;

class gameModel : public gameObject {
	public:
		typedef std::shared_ptr<gameModel> ptr;
		typedef std::weak_ptr<gameModel> weakptr;

		gameModel() : gameObject(objType::Model) {};

		virtual std::string typeString(void) {
			return "Model";
		}

		void genInfo(void);
		void genNormals(void);
		void genTexcoords(void);
		void genTangents(void);
		void genAABBs(void);

		std::string modelName = "unit_cube";
		// TODO: some sort of specifier for generated meshes
		std::string sourceFile = "";
		bool compiled = false;
		std::shared_ptr<compiledModel> comped_model;

		// TODO: allow attaching shaders to objects
		//Program::ptr shader = nullptr;

		// TODO: vertex data can be freed after compiling, can keep a
		//       reference to the compiled model, which would also make it
		//       much easier to free models from the GPU
		std::vector<glm::vec3> vertices;
		std::vector<glm::vec3> normals;
		std::vector<glm::vec3> tangents;
		std::vector<glm::vec2> texcoords;
		std::vector<usvec4>    joints;
		std::vector<glm::vec4> weights;

		// TODO: maybe materials can be subnodes...
		//std::map<std::string, material> materials;

		// used to determine if normals, etc need to be generated
		bool haveNormals = false;
		bool haveTexcoords = false;
		bool haveTangents = false;
		bool haveJoints = false;
		bool haveAABB = false;
};

typedef std::map<std::string, gameMesh::ptr> meshMap;
typedef std::map<std::string, gameModel::ptr> modelMap;

gameModel::ptr load_object(std::string filename);
std::map<std::string, material::ptr>
  load_materials(gameModel::ptr model, std::string filename);

modelMap load_gltf_models(std::string filename);
modelMap load_gltf_models(tinygltf::Model& tgltf_model);
std::pair<gameImport::ptr, modelMap> load_gltf_scene(std::string filename);
// TODO: load scene

// namespace grendx
}

// handle forward declaration of compiledModel
#include <grend/glManager.hpp>
