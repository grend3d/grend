#pragma once
#include <grend/sdl-context.hpp>
#include <grend/opengl-includes.hpp>
#include <grend/glm-includes.hpp>
#include <grend/model.hpp>

#include <vector>
#include <map>
#include <string>
#include <utility>

// TODO: expose this config somehow
#define ENABLE_MIPMAPS 1
#define ENABLE_FACE_CULLING 1

namespace grendx {

typedef std::map<std::string, model_submesh> mesh_map;
typedef std::map<std::string, model> model_map;


// TODO: need to split this up somehow
class grend {
	public:
		grend();
		~grend() { free_objects(); };

		// TODO: we could have a parent field so we can catch mismatched vbo->vao binds
		//       which could be neat
		typedef std::pair<GLuint, std::size_t> rhandle;

		class compiled_mesh {
			public:
				std::string material;
				rhandle vao;

				GLint elements_size;
				void *elements_offset;
		};
		typedef std::map<std::string, compiled_mesh> cooked_mesh_map;
		
		class compiled_model {
			public:
				rhandle vao;

				GLint vertices_size;
				GLint normals_size;
				GLint texcoords_size;
				GLint tangents_size;
				GLint bitangents_size;

				std::vector<std::string> meshes;
				// NOTE: duplicating materials here because the model may not be valid
				//       for the lifetime of the compiled model, and there's some possible
				//       optimizations to be done in buffering all the material info
				//       to the shaders during initialization
				std::map<std::string, material> materials;
				std::map<std::string, rhandle> mat_textures;

				void *vertices_offset;
				void *normals_offset;
				void *texcoords_offset;
				void *tangents_offset;
				void *bitangents_offset;
		};
		typedef std::map<std::string, compiled_model> cooked_model_map;

		void compile_meshes(std::string objname, const mesh_map& meshies);
		void compile_models(model_map& models);
		rhandle preload_mesh_vao(compiled_model& obj, compiled_mesh& mesh);
		rhandle preload_model_vao(compiled_model& mesh);
		void bind_cooked_meshes(void);

		cooked_mesh_map  cooked_meshes;
		cooked_model_map cooked_models;

		std::vector<glm::vec3> cooked_vertices;
		std::vector<glm::vec3> cooked_normals;
		std::vector<glm::vec3> cooked_tangents;
		std::vector<glm::vec3> cooked_bitangents;
		std::vector<GLushort> cooked_elements;
		std::vector<GLfloat> cooked_texcoords;

		rhandle cooked_vert_vbo;
		rhandle cooked_normal_vbo;
		rhandle cooked_tangent_vbo;
		rhandle cooked_bitangent_vbo;
		rhandle cooked_element_vbo;
		rhandle cooked_texcoord_vbo;

		rhandle gen_vao(void);
		rhandle gen_vbo(void);
		rhandle gen_texture(void);
		rhandle gen_shader(GLuint type);
		rhandle gen_program(void);

		rhandle bind_vao(const rhandle& handle);
		rhandle bind_vbo(const rhandle& handle, GLuint type);

		rhandle va_pointer(const rhandle& handle, GLuint width, GLuint type);
		rhandle enable_vbo(const rhandle& handle);

		rhandle buffer_vbo(const rhandle& handle, GLuint type, const std::vector<GLfloat>& vec);
		rhandle buffer_vbo(const rhandle& handle, GLuint type, const std::vector<GLushort>& vec);
		rhandle buffer_vbo(const rhandle& handle, GLuint type, const std::vector<glm::vec3>& vec);

		// XXX: yeah it's a bit obfuscated, but this turns a lot of boilerplate
		//      into just one function call
		// bind, load and enable vec3(float) attribute array buffer
		rhandle load_vec3f_ab_vattrib(const std::vector<GLfloat>& vec);
		rhandle load_vec3f_ab_vattrib(const std::vector<glm::vec3>& vec);
		// bind, load and enable vec3(unsigned short) attribute element array buffer
		rhandle load_vec3us_eab_vattrib(const std::vector<GLushort>& vec);

		void free_objects(void);

		// TODO: unload
		rhandle load_texture(std::string filename);
		rhandle load_shader(std::string filename, GLuint type);

		std::vector<GLuint> vaos;
		std::vector<GLuint> vbos;
		std::vector<GLuint> shaders;
		std::vector<GLuint> programs;
		std::vector<GLuint> textures;

		// frames rendered
		unsigned frames = 0;
		rhandle current_vao;
};

// namespace grendx
}
