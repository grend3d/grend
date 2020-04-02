#pragma once
#include <grend/glm-includes.hpp>
#include <grend/opengl-includes.hpp>
#include <string>
#include <vector>
#include <map>

namespace grendx {

struct material {
	glm::vec4 diffuse;
	glm::vec4 ambient;
	glm::vec4 specular;
	GLfloat   shininess = 10;
	GLfloat   opacity = 1.0;
	GLfloat   refract_idx = 1.5;

	// file names of textures
	// no ambient map, diffuse map serves as both
	std::string diffuse_map = "";
	std::string specular_map = "";
	std::string normal_map = "";
	std::string ambient_occ_map = "";
	std::string alpha_map = "";
};

class model_submesh {
	public:
		std::string material = "(null)";
		std::vector<GLushort> faces;
};

class model {
	public:
		model(){
			vertices.clear();
			normals.clear();
			tangents.clear();
			bitangents.clear();
			texcoords.clear();
		};
		model(std::string filename);
		void load_object(std::string filename);
		void load_materials(std::string filename);
		void clear(void);

		void gen_all(void);
		void gen_normals(void);
		void gen_texcoords(void);
		void gen_tangents(void);

		std::vector<glm::vec3> vertices;
		std::vector<glm::vec3> normals;
		std::vector<glm::vec3> tangents;
		std::vector<glm::vec3> bitangents;
		std::vector<glm::vec2> texcoords;

		std::map<std::string, model_submesh> meshes = {};
		std::map<std::string, material> materials = {};

		bool have_normals = false;
		bool have_texcoords = false;
};

typedef std::map<std::string, model_submesh> mesh_map;
typedef std::map<std::string, model> model_map;

model_map load_gltf_models(std::string filename);
// TODO: load scene

// namespace grendx
}
