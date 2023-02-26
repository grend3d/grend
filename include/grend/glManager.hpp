#pragma once

#include <grend-config.h>
#include <grend/glVersion.hpp>
#include <grend/sdlContext.hpp>
#include <grend/openglIncludes.hpp>
#include <grend/glmIncludes.hpp>
#include <grend/bufferAllocator.hpp>
#include <grend/textureData.hpp>

#include <vector>
#include <map>
#include <unordered_map>
#include <string>
#include <utility>
#include <set>
#include <memory>
#include <variant>

// XXX
#include <iostream>

// TODO: expose this config somehow
#define ENABLE_MIPMAPS 1
#define ENABLE_FACE_CULLING 1

namespace grendx {

void check_errors(int line, const char *filename, const char *func);

#if GREND_ERROR_CHECK
inline bool checkGLerrors = true;

#define DO_ERROR_CHECK() \
	{ if (checkGLerrors) { check_errors(__LINE__, __FILE__, __func__); }}

#define ENABLE_GL_ERROR_CHECK(STATE) \
	{ checkGLerrors = (STATE); }

#define GL_ERROR_CHECK_ENABLED() \
	(checkGLerrors)

#else
#define DO_ERROR_CHECK() /* asdf */
#define ENABLE_GL_ERROR_CHECK(STATE) /* blarg */
#define GL_ERROR_CHECK_ENABLED() (0)
#endif

// utility macros for setting entries in VAOs
#define STRUCT_OFFSET(TYPE, MEMBER) \
	((void *)&(((TYPE*)NULL)->MEMBER))

// constexpr, should never be evaluated at runtime
#define GLM_VEC_ENTRIES(TYPE, MEMBER) \
	((((TYPE*)NULL)->MEMBER).length())

#define SET_VAO_ENTRY(BINDING, TYPE, MEMBER) \
	do { \
	glVertexAttribPointer(BINDING, GLM_VEC_ENTRIES(TYPE, MEMBER), \
	                      GL_FLOAT, GL_FALSE, sizeof(TYPE),       \
	                      STRUCT_OFFSET(TYPE, MEMBER));           \
	} while(0);

enum {
	// VAO locations, used to link shaders to proper vao entries
	VAO_ELEMENTS      = 0,
	VAO_VERTICES      = 1,
	VAO_NORMALS       = 2,
	VAO_TANGENTS      = 3,
	VAO_COLORS        = 4,
	VAO_TEXCOORDS     = 5,
	VAO_LIGHTMAP      = 6,
	VAO_JOINTS        = 7,
	VAO_JOINT_WEIGHTS = 8,
};

enum {
	VAO_QUAD_VERTICES  = 0,
	VAO_QUAD_TEXCOORDS = 1,
};

// TODO: remove light info buffer
enum {
	UBO_LIGHT_INFO               = 2,
	UBO_POINT_LIGHT_TILES        = 3,
	UBO_SPOT_LIGHT_TILES         = 4,
	UBO_JOINTS                   = 5,
	UBO_INSTANCE_TRANSFORMS      = 6,
	UBO_POINT_LIGHT_BUFFER       = 7,
	UBO_SPOT_LIGHT_BUFFER        = 8,
	UBO_DIRECTIONAL_LIGHT_BUFFER = 9,
	UBO_END_BINDINGS,
};

enum {
	TEX_GL_SCRATCH     = GL_TEXTURE0,
	TEX_GL_SCRATCHB    = GL_TEXTURE1,
	TEX_GL_DIFFUSE     = GL_TEXTURE2,
	TEX_GL_METALROUGH  = GL_TEXTURE3,
	TEX_GL_NORMAL      = GL_TEXTURE4,
	TEX_GL_AO          = GL_TEXTURE5,
	TEX_GL_EMISSIVE    = GL_TEXTURE6,
	TEX_GL_LIGHTMAP    = GL_TEXTURE11,
	TEX_GL_REFLECTIONS = GL_TEXTURE7,
	TEX_GL_SHADOWS     = GL_TEXTURE8,
	TEX_GL_IRRADIANCE  = GL_TEXTURE9,
	TEX_GL_SKYBOX      = GL_TEXTURE10,
};

// Hmm... has to be a better way to do this
enum {
	TEXU_SCRATCH     = 0,
	TEXU_SCRATCHB    = 1,
	TEXU_DIFFUSE     = 2,
	TEXU_METALROUGH  = 3,
	TEXU_NORMAL      = 4,
	TEXU_AO          = 5,
	TEXU_EMISSIVE    = 6,
	TEXU_LIGHTMAP    = 11,
	TEXU_REFLECTIONS = 7,
	TEXU_SHADOWS     = 8,
	TEXU_IRRADIANCE  = 9,
	TEXU_SKYBOX      = 10,
	// TODO: automatic
	TEXU_MAX = 12,
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

		//void *map(GLenum access = GL_WRITE_ONLY);
		void *map(GLenum access);
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

		void buffer(textureData::ptr tex);
		void buffer(const textureData& tex);

		// TODO: don't like this class being responsible for loading data
		void cubemap(std::string directory, std::string extension=".jpg");

		void bind(GLenum target = GL_TEXTURE_2D) {
			glBindTexture(target, obj);
		}

		textureData::imageType type;
};

class Shader : public Obj {
	public:
		typedef std::shared_ptr<Shader> ptr;
		// one variant to rule them all
		typedef std::variant<
			GLint, GLfloat,
			glm::vec2, glm::vec3, glm::vec4,
			glm::mat2, glm::mat3, glm::mat4
		> value;

		typedef std::map<std::string, value> parameters;

		Shader(GLuint o);
		bool load(std::string path, const parameters& options);
		bool reload(void);
		std::string filepath = "";
		parameters compiledOptions;
};

// combines parameter maps together, entries later in the list will override
// entries earlier in the list (when they overlap)
Shader::parameters mergeOpts(const std::initializer_list<Shader::parameters>& opts);

class Program : public Obj {
	GLint linked = false;

	public:
		typedef std::shared_ptr<Program> ptr;
		typedef std::weak_ptr<Program>   weakptr;

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
		bool set(std::string uniform, GLint i);
		bool set(std::string uniform, GLfloat f);
		bool set(std::string uniform, glm::vec2 v2);
		bool set(std::string uniform, glm::vec3 v3);
		bool set(std::string uniform, glm::vec4 v4);
		bool set(std::string uniform, glm::mat3 m3);
		bool set(std::string uniform, glm::mat4 m4);
		bool set(std::string uniform, Shader::value val);

		bool setUniformBlock(std::string name, Buffer::ptr buf, GLuint binding);
		bool setStorageBlock(std::string name, Buffer::ptr buf, GLuint binding);

		GLint  lookup(std::string uniform);
		GLuint lookupUniformBlock(std::string name);
		GLuint lookupStorageBlock(std::string name);
		bool cached(std::string uniform);
		// TODO: valueCached, objectCached
		// TODO: value caching still not implemented, should do that,
		//       performance is becoming important
		bool cacheObject(const char *name, uintptr_t obj);
		bool cacheObject(const char *name, void *obj);

		// cache to keep track of objects set for this shader, eg.
		// eg. irradiance, reflection probes, expensive to test each value
		// for every mesh call compared to testing a pointer per mesh
		std::unordered_map<const char *, uintptr_t> objCache;

		std::map<std::string, GLint>  uniforms;
		std::map<std::string, GLuint> attributes;
		std::map<std::string, GLuint> uniformBlocks;
		std::map<std::string, GLuint> storageBlocks;

		Shader::parameters valueCache;
};

class Framebuffer : public Obj {
	public:
		typedef std::shared_ptr<Framebuffer> ptr;

		Framebuffer()         : Obj(0, Obj::type::Framebuffer) {};
		Framebuffer(GLuint o) : Obj(o, Obj::type::Framebuffer) {};

		void bind(void);
		Texture::ptr attach(GLenum attachment, Texture::ptr texture,
		                    GLenum textarget = GL_TEXTURE_2D);

		// to prevent attachments from being free'd
		std::map<GLenum, Texture::ptr> attachments;
};

Texture::ptr texcache(textureData::ptr tex);

void initializeOpengl(void);

Vao::ptr getCurrentVao(void);
Vao::ptr getScreenquadVao(void);
Vao::ptr bindVao(Vao::ptr vao);
void setFaceOrder(GLenum face_order);
void setDefaultGlFlags(void);

void enable(GLenum feature, bool ignoreCache = false);
void disable(GLenum feature, bool ignoreCache = false);

void preloadScreenquad(void);
void drawScreenquad(void);

Vao::ptr         genVao(void);
Buffer::ptr      genBuffer(GLuint type, GLenum use = GL_STATIC_DRAW);
Texture::ptr     genTexture(void);
Shader::ptr      genShader(GLuint type);
Program::ptr     genProgram(void);
Framebuffer::ptr genFramebuffer(void);

struct glTexFormat {
	GLenum internal;
	GLenum format;
	GLenum type;
	GLenum minfilter = 0;
	GLenum magfilter = 0;
};

bool haveFloatBuffers(void);

static inline glTexFormat rgbaf_if_supported(void) {
	return haveFloatBuffers()
		? glTexFormat {
			.internal = GL_RGBA16F,
			.format   = GL_RGBA,
			.type     = GL_FLOAT,
			.minfilter = GL_LINEAR,
			.magfilter = GL_LINEAR, }

		: glTexFormat {
			.internal  = GL_RGBA8,
			.format    = GL_RGBA,
			.type      = GL_UNSIGNED_BYTE,
			.minfilter = GL_LINEAR,
			.magfilter = GL_LINEAR,
		};
}

static inline glTexFormat rgbf_if_supported(void)  {
	return haveFloatBuffers()
		? glTexFormat {
			.internal = GL_RGBA16F,
			.format   = GL_RGBA,
			.type     = GL_FLOAT,
			.minfilter = GL_LINEAR,
			.magfilter = GL_LINEAR, }

		: glTexFormat {
			.internal  = GL_RGB8,
			.format    = GL_RGB,
			.type      = GL_UNSIGNED_BYTE,
			.minfilter = GL_LINEAR,
			.magfilter = GL_LINEAR, };
}

static inline glTexFormat depth_stencil_format(void) {
	return {
		.internal = GL_DEPTH24_STENCIL8,
		.format   = GL_DEPTH_STENCIL,
		.type     = GL_UNSIGNED_INT_24_8,
		.minfilter = GL_NEAREST,
		.magfilter = GL_NEAREST,
	};
}

// emscripten webgl layer doesn't define GL_R16
#if defined(GL_R16) && (GLSL_VERSION > 300)
#define INDEX_FORMAT_BITS 16
#else
#define INDEX_FORMAT_BITS 8
#endif

static inline glTexFormat index_format(void) {
#if INDEX_FORMAT_BITS == 16
	return {
		.internal = GL_R16,
		.format   = GL_RED,
		.type     = GL_UNSIGNED_SHORT,
	};
#else
	return {
		.internal = GL_R8,
		.format   = GL_RED,
		.type     = GL_UNSIGNED_BYTE,
	};
#endif
}

Texture::ptr genTextureFormat(unsigned width, unsigned height,
                              const glTexFormat& format);

#if defined(HAVE_MULTISAMPLE)
Texture::ptr genTextureFormatMS(unsigned width,
                                unsigned height,
                                unsigned samples,
                                const glTexFormat& format);
#endif

Program::ptr loadProgram(std::string vert,
                         std::string frag,
                         const Shader::parameters& opts);

GLenum surfaceGlFormat(SDL_Surface *surf);
GLenum surfaceGlFormat(int channels);
GLenum surfaceGlFormat(const textureData& tex);

// namespace grendx
}
