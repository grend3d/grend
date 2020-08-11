#pragma once
#include <grend/sdl-context.hpp>
#include <grend/opengl-includes.hpp>
#include <grend/glm-includes.hpp>
#include <grend/model.hpp>

#include <vector>
#include <map>
#include <string>
#include <utility>
#include <set>
#include <memory>

// XXX
#include <iostream>

// TODO: expose this config somehow
#define ENABLE_MIPMAPS 1
#define ENABLE_FACE_CULLING 1

namespace grendx {

void check_errors(int line, const char *func);

#ifdef NO_ERROR_CHECK
#define DO_ERROR_CHECK() /* asdf */

#else
#define DO_ERROR_CHECK() \
	{ check_errors(__LINE__, __func__); }
#endif

class Obj {
	public:
		enum type {
			None,
			Vao,
			Vbo,
			Texture,
			Shader,
			Program,
			Framebuffer,
		};

		Obj() : obj(0), objtype(type::None) {}
		Obj(GLuint o, enum type t) : obj(o), objtype(t) {}

		~Obj() {
			if (obj == 0) {
				// avoid freeing default objects
				return;
			}

			/*
			switch (objtype) {
				case type::Vao:         glDeleteVertexArrays(1, &obj); break;
				case type::Vbo:         glDeleteBuffers(1, &obj); break;
				case type::Texture:     glDeleteTextures(1, &obj); break;
				case type::Shader:      break; // TODO
				case type::Program:     glDeleteProgram(obj); break;
				case type::Framebuffer: glDeleteFramebuffers(1, &obj); break;

				default: break;
			}
			*/

			switch (objtype) {
				case type::Vao:
					std::cerr << "Obj: freeing Vao " << obj << std::endl;
					glDeleteVertexArrays(1, &obj);
					break;

				case type::Vbo:
					std::cerr << "Obj: freeing Vbo " << obj << std::endl;
					glDeleteBuffers(1, &obj);
					break;

				case type::Texture:
					std::cerr << "Obj: freeing Texture " << obj << std::endl;
					glDeleteTextures(1, &obj);
					break;

				case type::Shader:
					std::cerr << "Obj: freeing Shader " << obj << std::endl;
					break; // TODO

				case type::Program:
					std::cerr << "Obj: freeing Program " << obj << std::endl;
					glDeleteProgram(obj);
					break;

				case type::Framebuffer:
					std::cerr << "Obj: freeing Framebuffer " << obj << std::endl;
					glDeleteFramebuffers(1, &obj);
					break;

				default: break;
			}
		}

		GLuint obj;
		enum type objtype;
};

class Vao : public Obj {
	public:
		typedef std::shared_ptr<Vao> ptr;
		Vao(GLuint o) : Obj(o, Obj::type::Vao) {};
		void bind(void) {
			glBindVertexArray(obj);
		}
};

class Vbo : public Obj {
	public:
		typedef std::shared_ptr<Vbo> ptr;
		Vbo(GLuint o) : Obj(o, Obj::type::Vbo) {};

		void bind(GLuint place) {
			glBindBuffer(place, obj);
			DO_ERROR_CHECK();
		}

		void buffer(GLuint type, const std::vector<GLfloat>& vec,
		            GLenum use=GL_STATIC_DRAW);
		void buffer(GLuint type, const std::vector<GLushort>& vec,
		            GLenum use=GL_STATIC_DRAW);
		void buffer(GLuint type, const std::vector<GLuint>& vec,
		            GLenum use=GL_STATIC_DRAW);
		void buffer(GLuint type, const std::vector<glm::vec3>& vec,
		            GLenum use=GL_STATIC_DRAW);
};

class Texture : public Obj {
	public:
		typedef std::shared_ptr<Texture> ptr;
		typedef std::weak_ptr<Texture> weakptr;

		Texture(GLuint o) : Obj(o, Obj::type::Texture) {}
		void buffer(const material_texture& tex, bool srgb=false);
		void cubemap(std::string directory, std::string extension=".jpg");
		void bind(GLenum target = GL_TEXTURE_2D) {
			glBindTexture(target, obj);
		}
};

class Shader : public Obj {
	public:
		typedef std::shared_ptr<Shader> ptr;
		Shader(GLuint o, std::string p = "");
		bool load(std::string path);
		bool reload(void);
		std::string filepath;
};

class Program : public Obj {
	public:
		typedef std::shared_ptr<Program> ptr;
		Program(GLuint o) : Obj(o, Obj::type::Program) {}
		bool reload(void);

		Shader::ptr vertex, fragment;
		void bind(void) {
			glUseProgram(obj);
		}

		void set(std::string uniform, GLint i);
		void set(std::string uniform, GLfloat f);
		void set(std::string uniform, glm::vec2 v2);
		void set(std::string uniform, glm::vec3 v3);
		void set(std::string uniform, glm::vec4 v4);
		void set(std::string uniform, glm::mat3 m3);
		void set(std::string uniform, glm::mat4 m4);

		GLint lookup(std::string uniform);
		bool cached(std::string uniform);

		std::map<std::string, GLint> uniforms;
		// XXX: little bit redundant for caching values, but there
		//      should only be 10s of shader programs at most, and being
		//      strongly-typed is better than some union thing
		struct {
			std::map<GLint, GLint>     ints;
			std::map<GLint, GLfloat>   floats;
			std::map<GLint, glm::vec2> vec2s;
			std::map<GLint, glm::vec3> vec3s;
			std::map<GLint, glm::vec4> vec4s;
			std::map<GLint, glm::mat3> mat3s;
			std::map<GLint, glm::mat4> mat4s;
		} cache;
};

class Framebuffer : public Obj {
	public:
		typedef std::shared_ptr<Framebuffer> ptr;

		Framebuffer()         : Obj(0, Obj::type::Framebuffer) {};
		Framebuffer(GLuint o) : Obj(o, Obj::type::Framebuffer) {};

		void bind(void) {
			glBindFramebuffer(GL_FRAMEBUFFER, obj);
			DO_ERROR_CHECK();
		}

		Texture::ptr attach(GLenum attachment, Texture::ptr texture) {
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, texture->obj);
			glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D,
			                       texture->obj, 0);

			attachments[attachment] = texture;
			return texture;
		}

		// to prevent attachments from being free'd
		std::map<GLenum, Texture::ptr> attachments;
};

// TODO: need to split this up somehow
class gl_manager {
	public:
		gl_manager();

		class compiled_mesh {
			public:
				std::string material;
				Vao::ptr vao;

				GLint elements_size;
				void *elements_offset;
		};
		typedef std::map<std::string, compiled_mesh> cooked_mesh_map;
		
		class compiled_model {
			public:
				//rhandle vao;
				Vao::ptr vao;
				GLint vertices_size;

				std::vector<std::string> meshes;
				// NOTE: duplicating materials here because the model may not be valid
				//       for the lifetime of the compiled model, and there's some possible
				//       optimizations to be done in buffering all the material info
				//       to the shaders during initialization
				std::map<std::string, material> materials   = {};
				std::map<std::string, Texture::ptr> mat_textures = {};
				std::map<std::string, Texture::ptr> mat_specular = {};
				std::map<std::string, Texture::ptr> mat_normal   = {};
				std::map<std::string, Texture::ptr> mat_ao       = {};

				void *vertices_offset;
				void *normals_offset;
				void *texcoords_offset;
				void *tangents_offset;
				void *bitangents_offset;
		};
		typedef std::map<std::string, compiled_model> cooked_model_map;
		Texture::ptr texcache(const material_texture& tex, bool srgb = false);

		void compile_meshes(std::string objname, const mesh_map& meshies);
		void compile_model(std::string name, const model& mod);
		void compile_models(model_map& models);
		Vao::ptr preload_mesh_vao(compiled_model& obj, compiled_mesh& mesh);
		Vao::ptr preload_model_vao(compiled_model& mesh);
		void bind_cooked_meshes(void);

		void preload_screenquad(void);

		cooked_mesh_map  cooked_meshes;
		cooked_model_map cooked_models;

		std::vector<GLfloat> cooked_vertprops;
		std::vector<GLuint> cooked_elements;

		Vbo::ptr cooked_vertprops_vbo;
		Vbo::ptr cooked_element_vbo;
		Vbo::ptr screenquad_vbo;
		Vao::ptr screenquad_vao;

		//const shader& get_shader_obj(rhandle& handle);
		Vao::ptr bind_vao(Vao::ptr vao);
		void set_face_order(GLenum face_order);

		void enable(GLenum feature);
		void disable(GLenum feature);

		// cache of features currently enabled
		std::set<GLenum> feature_cache;

		// current state
		Vao::ptr current_vao;
		GLenum   current_face_order;

	private:
		std::map<uint32_t, Texture::weakptr> texture_cache;
};

Vao::ptr         gen_vao(void);
Vbo::ptr         gen_vbo(void);
Texture::ptr     gen_texture(void);
Shader::ptr      gen_shader(GLuint type);
Program::ptr     gen_program(void);
Framebuffer::ptr gen_framebuffer(void);

Texture::ptr gen_texture_color(unsigned width, unsigned height,
							   GLenum format=GL_RGBA);
Texture::ptr gen_texture_depth_stencil(unsigned width, unsigned height,
									   GLenum format=GL_DEPTH24_STENCIL8);

Program::ptr load_program(std::string vert, std::string frag);
Program::ptr link_program(Program::ptr program);

GLenum surface_gl_format(SDL_Surface *surf);
GLenum surface_gl_format(int channels);
GLenum surface_gl_format(const material_texture& tex);

// namespace grendx
}
