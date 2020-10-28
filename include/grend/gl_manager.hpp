#pragma once
// for forward declaration of gameModel
#include <grend/gameModel.hpp>
#include <grend/sdl-context.hpp>
#include <grend/opengl-includes.hpp>
#include <grend/glm-includes.hpp>

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

#if GLSL_VERSION == 100
#define GLSL_STRING "100"
#elif GLSL_VERSION == 300
#define GLSL_STRING "300 es"
#elif GLSL_VERSION == 330
#define GLSL_STRING "330 core"
#elif GLSL_VERSION == 430
#define GLSL_STRING "430 core"
#endif

namespace grendx {

void check_errors(int line, const char *filename, const char *func);

#if GREND_ERROR_CHECK
#define DO_ERROR_CHECK() \
	{ check_errors(__LINE__, __FILE__, __func__); }

#else
#define DO_ERROR_CHECK() /* asdf */
#endif

enum {
	// VAO locations, used to link shaders to proper vao entries
	VAO_ELEMENTS      = 0,
	VAO_VERTICES      = 1,
	VAO_NORMALS       = 2,
	VAO_TANGENTS      = 3,
	VAO_BITANGENTS    = 4,
	VAO_TEXCOORDS     = 5,
	VAO_JOINTS        = 6,
	VAO_JOINT_WEIGHTS = 7,
};

enum {
	VAO_QUAD_VERTICES  = 0,
	VAO_QUAD_TEXCOORDS = 1,
};

// XXX: metrics for debugging/optimization
// TODO: not global variables
inline size_t dbgGlmanBuffered = 0;
inline size_t dbgGlmanTexturesBuffered = 0;

static inline size_t glmanDbgUpdateBuffered(size_t old, size_t thenew) {
	dbgGlmanBuffered -= old;
	dbgGlmanBuffered += thenew;
	return thenew;
}

static inline size_t glmanDbgUpdateTextures(size_t old, size_t thenew) {
	dbgGlmanTexturesBuffered -= old;
	dbgGlmanTexturesBuffered += thenew;
	return thenew;
}

class Obj {
	public:
		enum type {
			None,
			Vao,
			Buffer,
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

				case type::Buffer:
					std::cerr << "Obj: freeing Buffer " << obj << std::endl;
					glDeleteBuffers(1, &obj);
					dbgGlmanBuffered -= currentSize;
					break;

				case type::Texture:
					std::cerr << "Obj: freeing Texture " << obj << std::endl;
					glDeleteTextures(1, &obj);
					dbgGlmanTexturesBuffered -= currentSize;
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

		// XXX: used to keep track of how much memory sent to the GPU
		//      (see glman... variables at top)
		size_t currentSize = 0;
};

class Vao : public Obj {
	public:
		typedef std::shared_ptr<Vao> ptr;
		Vao(GLuint o) : Obj(o, Obj::type::Vao) {};
		void bind(void) {
			glBindVertexArray(obj);
		}
};

class Buffer : public Obj {
	public:
		GLuint type;
		GLuint use = GL_STATIC_DRAW;

		typedef std::shared_ptr<Buffer> ptr;
		Buffer(GLuint o, GLuint t, GLenum u = GL_STATIC_DRAW)
			: Obj(o, Obj::type::Buffer), type(t), use(u) {};

		void bind(void);
		void unbind(void);

		void *map(GLenum access = GL_WRITE_ONLY);
		GLenum unmap(void);

		void allocate(size_t n);
		void update(const void *ptr, size_t offset, size_t n);
		void buffer(const void *ptr, size_t n);
		void buffer(const std::vector<GLfloat>& vec);
		void buffer(const std::vector<GLushort>& vec);
		void buffer(const std::vector<GLuint>& vec);
		void buffer(const std::vector<glm::vec3>& vec);
};

class Texture : public Obj {
	public:
		typedef std::shared_ptr<Texture> ptr;
		typedef std::weak_ptr<Texture> weakptr;

		Texture(GLuint o) : Obj(o, Obj::type::Texture) {}
		void buffer(const materialTexture& tex, bool srgb=false);
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
	GLint linked = false;

	public:
		typedef std::shared_ptr<Program> ptr;
		Program(GLuint o) : Obj(o, Obj::type::Program) {}

		bool good(void) { return linked; };
		bool reload(void);
		bool link(void);
		std::string log(void);

		Shader::ptr vertex, fragment;
		void bind(void) {
			glUseProgram(obj);
		}

		void attribute(std::string attr, GLuint location);
		void set(std::string uniform, GLint i);
		void set(std::string uniform, GLfloat f);
		void set(std::string uniform, glm::vec2 v2);
		void set(std::string uniform, glm::vec3 v3);
		void set(std::string uniform, glm::vec4 v4);
		void set(std::string uniform, glm::mat3 m3);
		void set(std::string uniform, glm::mat4 m4);
		void setUniformBlock(std::string name, Buffer::ptr buf);
		void setStorageBlock(std::string name, Buffer::ptr buf);

		GLint  lookup(std::string uniform);
		GLuint lookupUniformBlock(std::string name);
		GLuint lookupStorageBlock(std::string name);
		bool cached(std::string uniform);

		std::map<std::string, GLint> uniforms;
		std::map<std::string, GLuint> attributes;
		std::map<std::string, GLuint> uniformBlocks;
		std::map<std::string, GLuint> storageBlocks;

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

// TODO: camelCase
class compiled_mesh {
	public:
		typedef std::shared_ptr<compiled_mesh> ptr;
		typedef std::weak_ptr<compiled_mesh> weakptr;

		std::string material;
		Vao::ptr vao;

		GLint elements_size;
		void *elements_offset;
};

// TODO: camelCase
class compiled_model {
	public:
		typedef std::shared_ptr<compiled_model> ptr;
		typedef std::weak_ptr<compiled_model> weakptr;

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
		std::map<std::string, Texture::ptr> mat_emissive = {};

		void *vertices_offset;
		void *normals_offset;
		void *texcoords_offset;
		void *tangents_offset;
		void *bitangents_offset;

		bool haveJoints = false;
		void *joints_offset;
		void *weights_offset;
};

// TODO: weakptr, once model loading stuff is straightened out
typedef std::map<std::string, compiled_mesh::ptr> cooked_mesh_map;
typedef std::map<std::string, compiled_model::ptr> cooked_model_map;
Texture::ptr texcache(const materialTexture& tex, bool srgb = false);

// defined in gameModel.hpp
class gameModel;

void initialize_opengl(void);
void compile_meshes(std::string objname, mesh_map& meshies);
void compile_model(std::string name, std::shared_ptr<gameModel> mod);
void compile_models(model_map& models);
Vao::ptr preload_mesh_vao(compiled_model::ptr obj, compiled_mesh& mesh);
Vao::ptr preload_model_vao(compiled_model::ptr mesh);
void bind_cooked_meshes(void);
Vao::ptr get_current_vao(void);
Vao::ptr get_screenquad_vao(void);
Vao::ptr bind_vao(Vao::ptr vao);
void set_face_order(GLenum face_order);
void set_default_gl_flags(void);

void enable(GLenum feature);
void disable(GLenum feature);

void preload_screenquad(void);
void draw_screenquad(void);

Vao::ptr         gen_vao(void);
Buffer::ptr      gen_buffer(GLuint type, GLenum use = GL_STATIC_DRAW);
Texture::ptr     gen_texture(void);
Shader::ptr      gen_shader(GLuint type);
Program::ptr     gen_program(void);
Framebuffer::ptr gen_framebuffer(void);

// TODO: defining these as functions to leave open the possibility of
//       determining this at runtime
#ifdef NO_FLOATING_FB
static inline GLenum rgbaf_if_supported(void) { return GL_RGBA; }
#else
static inline GLenum rgbaf_if_supported(void) { return GL_RGBA16F; }
#endif

Texture::ptr gen_texture_color(unsigned width, unsigned height,
							   GLenum format=GL_RGBA);
Texture::ptr gen_texture_depth_stencil(unsigned width, unsigned height,
									   GLenum format=GL_DEPTH24_STENCIL8);

Program::ptr load_program(std::string vert, std::string frag);

GLenum surface_gl_format(SDL_Surface *surf);
GLenum surface_gl_format(int channels);
GLenum surface_gl_format(const materialTexture& tex);

// namespace grendx
}

