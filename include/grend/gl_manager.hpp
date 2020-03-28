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
class gl_manager {
	public:
		gl_manager();
		~gl_manager() { free_objects(); };

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

				std::vector<std::string> meshes;
				// NOTE: duplicating materials here because the model may not be valid
				//       for the lifetime of the compiled model, and there's some possible
				//       optimizations to be done in buffering all the material info
				//       to the shaders during initialization
				std::map<std::string, material> materials   = {};
				std::map<std::string, rhandle> mat_textures = {};
				std::map<std::string, rhandle> mat_specular = {};
				std::map<std::string, rhandle> mat_normal   = {};
				std::map<std::string, rhandle> mat_ao       = {};

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

		void preload_screenquad(void);

		cooked_mesh_map  cooked_meshes;
		cooked_model_map cooked_models;

		std::vector<GLfloat> cooked_vertprops;
		/*
		std::vector<GLfloat> cooked_vertices;
		std::vector<glm::vec3> cooked_normals;
		std::vector<glm::vec3> cooked_tangents;
		std::vector<glm::vec3> cooked_bitangents;
		std::vector<GLfloat> cooked_texcoords;
		*/
		std::vector<GLushort> cooked_elements;

		rhandle cooked_vertprops_vbo;
		rhandle cooked_element_vbo;

		/*
		rhandle cooked_vert_vbo;
		rhandle cooked_normal_vbo;
		rhandle cooked_tangent_vbo;
		rhandle cooked_bitangent_vbo;
		rhandle cooked_texcoord_vbo;
		*/

		rhandle screenquad_vbo;
		rhandle screenquad_vao;

		rhandle gen_vao(void);
		rhandle gen_vbo(void);
		rhandle gen_texture(void);
		rhandle gen_shader(GLuint type);
		rhandle gen_program(void);
		rhandle gen_framebuffer(void);

		rhandle gen_texture_color(unsigned width, unsigned height,
		                          GLenum format=GL_RGBA);
		rhandle gen_texture_depth_stencil(unsigned width, unsigned height,
		                                  GLenum format=GL_DEPTH24_STENCIL8);

		rhandle bind_vao(const rhandle& handle);
		rhandle bind_vbo(const rhandle& handle, GLuint type);
		rhandle bind_framebuffer(const rhandle& handle);
		void    bind_default_framebuffer(void);

		rhandle va_pointer(const rhandle& handle, GLuint width, GLuint type);
		//rhandle enable_vbo(const rhandle& handle);

		rhandle buffer_vbo(const rhandle& handle, GLuint type, const std::vector<GLfloat>& vec, GLenum usage=GL_STATIC_DRAW);
		rhandle buffer_vbo(const rhandle& handle, GLuint type, const std::vector<GLushort>& vec, GLenum usage=GL_STATIC_DRAW);
		rhandle buffer_vbo(const rhandle& handle, GLuint type, const std::vector<glm::vec3>& vec, GLenum usage=GL_STATIC_DRAW);

		void free_objects(void);

		// TODO: unload
		rhandle load_texture(std::string filename, bool srgb=false);
		rhandle load_cubemap(std::string directory, std::string extension=".jpg");
		rhandle load_shader(std::string filename, GLuint type);
		rhandle fb_attach_texture(GLenum attachment, const rhandle& texture);

		// map of loaded textures by filename
		std::map<std::string, rhandle> texture_cache;

		std::vector<GLuint> vaos;
		std::vector<GLuint> vbos;
		std::vector<GLuint> shaders;
		std::vector<GLuint> programs;
		std::vector<GLuint> textures;
		std::vector<GLuint> framebuffers;

		// frames rendered
		unsigned frames = 0;
		rhandle current_vao;
};

GLenum surface_gl_format(SDL_Surface *surf);
void check_errors(int line, const char *func);
#define DO_ERROR_CHECK() \
	{ check_errors(__LINE__, __func__); }

// namespace grendx
}
