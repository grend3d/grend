#pragma once

#include <grend/gameObject.hpp>
#include <grend/glm-includes.hpp>
#include <grend/opengl-includes.hpp>
#include <grend/scene.hpp>
#include <string>
#include <vector>
#include <map>
#include <tinygltf/tiny_gltf.h>

#include <stdint.h>

namespace grendx {

// TODO: need a material cache, having a way to lazily load/unload texture
//       would be a very good thing
class materialTexture {
	public:
		materialTexture() { };
		materialTexture(std::string filename);
		void load_texture(std::string filename);
		bool loaded(void) const { return channels != 0; };

		int width = 0, height = 0;
		int channels = 0;
		size_t size;
		std::vector<uint8_t> pixels;
};

struct material {
	enum blend_mode {
		Opaque,
		Mask,
		Blend,
	};

	glm::vec4 diffuse;
	glm::vec4 ambient;
	glm::vec4 specular;
	GLfloat   roughness = 0.5;
	GLfloat   metalness = 0.5;
	GLfloat   opacity = 1.0;
	GLfloat   refract_idx = 1.5;
	enum blend_mode blend = blend_mode::Opaque;

	void copy_properties(const struct material& other) {
		// copy everything but textures
		diffuse = other.diffuse;
		ambient = other.ambient;
		specular = other.specular;
		roughness = other.roughness;
		opacity = other.opacity;
		refract_idx = other.refract_idx;
		blend = other.blend;
	}

	materialTexture diffuse_map;
	materialTexture metal_roughness_map;
	materialTexture normal_map;
	materialTexture ambient_occ_map;
};

// TODO: camelCase
// defined in gl_manager.hpp
class compiled_mesh;
class compiled_model;

class gameMesh : public gameObject {
	public:
		typedef std::shared_ptr<gameMesh> ptr;
		typedef std::weak_ptr<gameMesh> weakptr;

		gameMesh() : gameObject(objType::Mesh) {};

		virtual std::string typeString(void) {
			std::stringstream strm;
			strm << "[Mesh " << meshName << " 0x" << std::hex << this <<  "]";
			return strm.str();
		}

		std::shared_ptr<compiled_mesh> comped_mesh;
		bool compiled = false;

		std::string meshName = "unit_cube:default";
		std::string material = "(null)";
		std::vector<GLuint> faces;
};

class gameModel : public gameObject {
	public:
		typedef std::shared_ptr<gameModel> ptr;
		typedef std::weak_ptr<gameModel> weakptr;

		gameModel() : gameObject(objType::Model) {};

		virtual std::string typeString(void) {
			std::stringstream strm;
			strm << "[Model " << modelName << " 0x" << std::hex << this <<  "]";
			return strm.str();
		}

		void genInfo(void);
		void genNormals(void);
		void genTexcoords(void);
		void genTangents(void);

		std::string modelName = "unit_cube";
		bool compiled = false;
		std::shared_ptr<compiled_model> comped_model;

		// TODO: allow attaching shaders to objects
		//Program::ptr shader = nullptr;

		// TODO: vertex data can be freed after compiling, can keep a
		//       reference to the compiled model, which would also make it
		//       much easier to free models from the GPU
		std::vector<glm::vec3> vertices;
		std::vector<glm::vec3> normals;
		std::vector<glm::vec3> tangents;
		std::vector<glm::vec3> bitangents;
		std::vector<glm::vec2> texcoords;

		// TODO: maybe materials can be subnodes...
		std::map<std::string, material> materials;

		// used to determine if normals, etc need to be generated
		bool haveNormals = false;
		bool haveTexcoords = false;
		bool haveTangents = false;
};

typedef std::map<std::string, gameMesh::ptr> mesh_map;
typedef std::map<std::string, gameModel::ptr> model_map;

gameModel::ptr load_object(std::string filename);
void           load_materials(gameModel::ptr model, std::string filename);
model_map load_gltf_models(std::string filename);
model_map load_gltf_models(tinygltf::Model& tgltf_model);
std::pair<scene, model_map> load_gltf_scene(std::string filename);
// TODO: load scene

// namespace grendx
}

// handle forward declaration of compiled_model
#include <grend/gl_manager.hpp>
