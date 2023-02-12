#include <grend/glManager.hpp>
#include <grend/sceneModel.hpp>
#include <grend/logger.hpp>

#include <SDL.h>
#include <string.h>

#include <string>
#include <vector>
#include <map>

#include <fstream>
#include <iostream>
#include <sstream>

namespace grendx {

static Buffer::ptr screenquadVbo;
static Vao::ptr screenquadVao;

// cache of features currently enabled
static std::set<GLenum> featureCache;

// current state
static Vao::ptr currentVao;
static GLenum   currentFaceOrder;

static std::map<uint32_t, Texture::weakptr>           textureCache;

static bool enabled_float_buffers = false;
static bool enabled_halffloat_buffers = false;

bool haveFloatBuffers(void) {
#if defined(CORE_FLOATING_POINT_BUFFERS)
	return true;
#else
	return enabled_float_buffers;
#endif
}

void initializeOpengl(void) {
	int maxImageUnits = 0;
	int maxCombined = 0;
	int maxVertexUniforms = 0;
	int maxFragmentUniforms = 0;
	int maxTextureSize = 0;
	int maxUBOBindings = 0;
	int maxUBOSize = 0;
	int maxFramebufAttachments = 0;
	int numExtensions = 0;

	glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS,          &maxImageUnits);
	glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &maxCombined);
	glGetIntegerv(GL_MAX_VERTEX_UNIFORM_VECTORS,       &maxVertexUniforms);
	glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_VECTORS,     &maxFragmentUniforms);
	glGetIntegerv(GL_MAX_TEXTURE_SIZE,                 &maxTextureSize);
	glGetIntegerv(GL_NUM_EXTENSIONS,                   &numExtensions);

#if GLSL_VERSION >= 140 /* OpenGL 3.1+ */
	glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS,            &maxFramebufAttachments);
	glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_BLOCKS,      &maxUBOBindings);
	glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE,           &maxUBOSize);
#endif
	DO_ERROR_CHECK();

	LogFmt(" OpenGL initializing... ");
	LogFmt(" OpenGL {}", (const char*)glGetString(GL_VERSION));
	LogFmt(" OpenGL vendor: {}", (const char *)glGetString(GL_VENDOR));
	LogFmt(" OpenGL renderer: {}", (const char *)glGetString(GL_RENDERER));

	LogFmt(" OpenGL GLSL target: {}", GLSL_VERSION);
	LogFmt(" OpenGL max fragment textures: {}", maxImageUnits);
	LogFmt(" OpenGL max total textures: {}", maxCombined);
	LogFmt(" OpenGL max texture size: {}", maxTextureSize);
	LogFmt(" OpenGL max vertex vector uniforms: {}", maxVertexUniforms);
	LogFmt(" OpenGL max fragment vector uniforms: {}", maxFragmentUniforms);
	LogFmt(" OpenGL max uniform block bindings: {}", maxUBOBindings);
	LogFmt(" OpenGL max uniform block size: {}", maxUBOSize);
	LogFmt(" OpenGL max framebuffer attachments: {}", maxFramebufAttachments);

	for (int i = 0; i < numExtensions; i++) {
		const char *str = (const char *)glGetStringi(GL_EXTENSIONS, i);
		LogFmt("Extension {}: {}", i, str);

		if (strcmp(str, "EXT_color_buffer_float") == 0)
			enabled_float_buffers = true;

		if (strcmp(str, "EXT_color_buffer_half_float") == 0)
			enabled_halffloat_buffers = true;
	}

	if (maxImageUnits < TEXU_MAX) {
		throw std::logic_error("This GPU doesn't allow enough texture bindings!");
	}

	if (maxTextureSize < 4096) {
		// TODO: this doesn't need to be an error, but also pretty much everything
		//       should support this
		throw std::logic_error("This GPU doesn't support 4096+ textures!");
	}

	// TODO: check for some minimum number of available uniforms

#if GLSL_VERSION >= 140
	if (maxUBOBindings < UBO_END_BINDINGS) {
		throw std::logic_error("This GPU doesn't allow enough uniform block bindings!");
	}
#endif

	// make sure we start with a bound VAO
	bindVao(genVao());

	// initialize state for quad that fills the screen
	preloadScreenquad();

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

// TODO: is this worth putting in it's own file?
void Framebuffer::bind(void) {
	glBindFramebuffer(GL_FRAMEBUFFER, obj);
	DO_ERROR_CHECK();
}

Texture::ptr Framebuffer::attach(GLenum attachment,
                                 Texture::ptr texture,
                                 GLenum textarget)
{
	glActiveTexture(TEX_GL_SCRATCH);
	glBindTexture(textarget, texture->obj);
	glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, textarget, texture->obj, 0);
	DO_ERROR_CHECK();

	attachments[attachment] = texture;
	return texture;
}

static inline uint32_t dumbhash(const std::vector<uint8_t>& pixels) {
	uint32_t ret = 1543;

	unsigned sample_offset;
	unsigned tag = 0;
	unsigned sq = sqrt(pixels.size()/4 /* assume RGBA */);

	// choose sample offsets to keep sample rate roughly consistent
	// between different texture sizes, smaller textures need smaller offsets
	// `tag` encodes the texture size in the upper two bits of the hash
	if (sq <= 256) {
		sample_offset = 593;
		tag = 0;

	} else if (sq <= 512) {
		sample_offset = 1031;
		tag = 1;

	} else if (sq <= 2048) {
		sample_offset = 4159;
		tag = 2;

	} else {
		sample_offset = 8101;
		tag = 3;
	}

	for (unsigned i = 0; i < pixels.size(); i += sample_offset) {
		ret = ((ret << 7) + ret + pixels[i]);
	}

	return (tag << 30) | (ret & ((1 << 30) - 1));
}

Texture::ptr texcache(textureData::ptr tex, bool srgb) {
	if (!tex || !tex->loaded()) {
		return nullptr;
	}

	// 0 is an invalid hash
	uint32_t hash = 0;

	if (auto *pixels = std::get_if<std::vector<uint8_t>>(&tex->pixels)) {
		hash = dumbhash(*pixels);
		auto it = textureCache.find(hash);

		if (it != textureCache.end()) {
			if (auto observe = it->second.lock()) {
				//static const unsigned sizes[4] = {256, 512, 2048, 4096};
				//logfmt("texture cache hit: %08x (%uK)\n", hash, sizes[hash >> 30]);
				return observe;
			}
		}

		// TODO: hashing for other pixel formats

	}

	Texture::ptr ret = genTexture();
	ret->buffer(tex, srgb);

	if (hash) {
		textureCache[hash] = ret;
	}

	return ret;
}

struct quadent {
	glm::vec3 position;
	glm::vec2 uv;
};

static std::vector<GLfloat> screenquad_data = {
	-1, -1,  0, 0, 0,
	 1, -1,  0, 1, 0,
	 1,  1,  0, 1, 1,

	 1,  1,  0, 1, 1,
	-1,  1,  0, 0, 1,
	-1, -1,  0, 0, 0,
};

void preloadScreenquad(void) {
	Vao::ptr orig_vao = currentVao;
	screenquadVao = bindVao(genVao());
	screenquadVbo = genBuffer(GL_ARRAY_BUFFER);

	screenquadVbo->bind();
	screenquadVbo->buffer(screenquad_data);
	glEnableVertexAttribArray(VAO_QUAD_VERTICES);
	SET_VAO_ENTRY(VAO_QUAD_VERTICES, quadent, position);

	glEnableVertexAttribArray(VAO_QUAD_TEXCOORDS);
	SET_VAO_ENTRY(VAO_QUAD_TEXCOORDS, quadent, uv);

	bindVao(orig_vao);
}

void drawScreenquad(void) {
	bindVao(getScreenquadVao());
	glDrawArrays(GL_TRIANGLES, 0, 6);
	DO_ERROR_CHECK();
}

void check_errors(int line, const char *filename, const char *func) {
	GLenum err;

	// TODO: maybe exceptions or some way to address errors
	while ((err = glGetError()) != GL_NO_ERROR) {
		const char *errstr;

		switch (err) {
			case GL_INVALID_ENUM:     
				errstr = "invalid enum"; break;
			case GL_INVALID_VALUE:    
				errstr = "invalid value"; break;
			case GL_INVALID_OPERATION:
				errstr = "invalid operation"; break;
			case GL_INVALID_FRAMEBUFFER_OPERATION:
				errstr = "invalid framebuffer operation"; break;
			case GL_OUT_OF_MEMORY:
				errstr = "out of memory"; break;
				/*
			case GL_STACK_UNDERFLOW:
				errstr = "stack underflow"; break;
			case GL_STACK_OVERFLOW:
				errstr = "stack overflow"; break;
				*/
			default:
				errstr = "some kind of error code"; break;
		}

		LogErrorFmt("/!\\ OPENGL ERROR: {}:{}: {}(): {} (errno. {})",
		            filename, line, func, errstr, err);

		// TODO: better abort
		*(int*)NULL = 1234;
	}
}

// TODO: camelCase
Vao::ptr genVao(void) {
	GLuint temp;
	glGenVertexArrays(1, &temp);
	DO_ERROR_CHECK();
	return Vao::ptr(new Vao(temp));
}

Buffer::ptr genBuffer(GLuint type, GLenum use) {
	GLuint temp;
	glGenBuffers(1, &temp);
	return std::make_shared<Buffer>(temp, type, use);
}

Texture::ptr genTexture(void) {
	GLuint temp;
	glGenTextures(1, &temp);
	DO_ERROR_CHECK();
	return Texture::ptr(new Texture(temp));
}

Shader::ptr genShader(GLuint type) {
	GLuint temp = glCreateShader(type);
	DO_ERROR_CHECK();
	return Shader::ptr(new Shader(temp));
}

Program::ptr genProgram(void) {
	GLuint temp = glCreateProgram();
	DO_ERROR_CHECK();
	return Program::ptr(new Program(temp));
}

Framebuffer::ptr genFramebuffer(void) {
	GLuint temp;
	glGenFramebuffers(1, &temp);
	DO_ERROR_CHECK();
	return Framebuffer::ptr(new Framebuffer(temp));
}

Texture::ptr genTextureFormat(unsigned width, unsigned height,
                              const glTexFormat& format)
{
	Texture::ptr ret = genTexture();

	glBindTexture(GL_TEXTURE_2D, ret->obj);
	glTexImage2D(GL_TEXTURE_2D, 0, format.internal, width, height, 0,
	             format.format, format.type, nullptr);

	if (format.minfilter) {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, format.minfilter);
	}

	if (format.magfilter) {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, format.magfilter);
	}

	glBindTexture(GL_TEXTURE_2D, 0);

	// assume float16 rgba format, 2 bytes
	size_t texsize = 4*2*width*height;
	ret->currentSize = glmanDbgUpdateTextures(ret->currentSize, texsize);

	return ret;
}

#if defined(HAVE_MULTISAMPLE)
Texture::ptr genTextureFormatMS(unsigned width,
                                unsigned height,
                                unsigned samples,
                                const glTexFormat& format)
{
	DO_ERROR_CHECK();
	Texture::ptr ret = genTexture();

	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, ret->obj);
	glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE,
			samples,
			format.internal,
			width, height, GL_FALSE);

	// assume float16 rgba format, 2 bytes
	size_t texsize = 4*2*samples*width*height;
	ret->currentSize = glmanDbgUpdateTextures(ret->currentSize, texsize);

	DO_ERROR_CHECK();
	return ret;
}
#endif

Vao::ptr getCurrentVao(void) {
	return currentVao;
}

Vao::ptr getScreenquadVao(void) {
	return screenquadVao;
}

Vao::ptr bindVao(Vao::ptr v) {
	currentVao = v;
	v->bind();
	DO_ERROR_CHECK();
	return v;
}

GLenum surfaceGlFormat(SDL_Surface *surf) {
	return surfaceGlFormat(surf->format->BytesPerPixel);
}

GLenum surfaceGlFormat(const textureData& tex) {
	return surfaceGlFormat(tex.channels);
}

GLenum surfaceGlFormat(int channels) {
	switch (channels) {
		case 1: return GL_RED;
		case 2: return GL_RG;
		case 3: return GL_RGB;
		case 4: return GL_RGBA;
		default: break;
	}

	return GL_RGBA;
}

void setFaceOrder(GLenum face_order) {
	if (face_order != currentFaceOrder) {
		currentFaceOrder = face_order;
		glFrontFace(face_order);
	}
}

void setDefaultGlFlags(void) {
	glDepthMask(GL_TRUE);
	glDepthFunc(GL_LESS);
	glStencilMask(~0);
	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
	glFrontFace(GL_CCW);
	glCullFace(GL_BACK);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glStencilFunc(GL_ALWAYS, 0, ~0);

	disable(GL_CULL_FACE, true);
	disable(GL_SCISSOR_TEST, true);
	disable(GL_STENCIL_TEST, true);
	enable(GL_BLEND, true);
	enable(GL_DEPTH_TEST, true);
	enable(GL_CULL_FACE, true);;
	// TODO: other flags

	glActiveTexture(TEX_GL_SCRATCH);
	glBindTexture(GL_TEXTURE_2D, TEXU_SCRATCH);
	glBindVertexArray(0);
}

void enable(GLenum feature, bool ignoreCache) {
	if (ignoreCache || featureCache.find(feature) == featureCache.end()) {
		glEnable(feature);
		featureCache.insert(feature);
	}
}

void disable(GLenum feature, bool ignoreCache) {
	auto it = featureCache.find(feature);
	bool active = it != featureCache.end();

	if (ignoreCache || active) {
		glDisable(feature);

		if (active) {
			featureCache.erase(it);
		}
	}
}

// namespace grendx
}
