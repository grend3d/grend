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

#include <stdint.h>

namespace grendx {

// defined in glManager.hpp
class compiledMesh;
class compiledModel;

class sceneMesh : public sceneNode {
	public:
		typedef std::shared_ptr<sceneMesh> ptr;
		typedef std::weak_ptr<sceneMesh> weakptr;

		sceneMesh() : sceneNode(objType::Mesh) {};
		virtual ~sceneMesh();

		virtual std::string typeString(void) {
			return "Mesh";
		}

		std::shared_ptr<compiledMesh> comped_mesh;
		bool compiled = false;

		std::string meshName = "unit_cube:default";
		material::ptr meshMaterial;
		std::vector<GLuint> faces;
		struct AABB boundingBox;
};

// used for joint indices
typedef glm::vec<4, uint16_t, glm::defaultp> usvec4;
typedef glm::vec<4, uint8_t,  glm::defaultp> ubvec4;

class sceneModel : public sceneNode {
	public:
		typedef std::shared_ptr<sceneModel> ptr;
		typedef std::weak_ptr<sceneModel> weakptr;

		sceneModel() : sceneNode(objType::Model) {};
		virtual ~sceneModel();

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
		//
		// TODO: vertex data can be freed after compiling, can keep a
		//       reference to the compiled model, which would also make it
		//       much easier to free models from the GPU

		struct vertex {
			glm::vec3 position;
			glm::vec3 normal;
			glm::vec4 tangent;
			glm::vec3 color;
			glm::vec2 uv;
			glm::vec2 lightmap;
		};

		std::vector<vertex> vertices;

		struct jointWeights {
			glm::vec4 joints;  // joints that affect the vertex
			glm::vec4 weights; // how much the joint affects the vertex
		};

		std::vector<jointWeights> joints;

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
std::map<std::string, material::ptr>
  load_materials(sceneModel::ptr model, std::string filename);

modelMap load_gltf_models(std::string filename);
std::pair<sceneImport::ptr, modelMap> load_gltf_scene(std::string filename);
// TODO: load scene

// namespace grendx
}

// handle forward declaration of compiledModel
#include <grend/glManager.hpp>
