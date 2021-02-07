#include <grend/glManager.hpp>
#include <grend/gameModel.hpp>
#include <SDL.h>

#include <string>
#include <vector>
#include <map>

#include <fstream>
#include <iostream>
#include <sstream>

namespace grendx {

static cookedMeshMap  cookedMeshes;
static cookedModelMap cookedModels;

static Buffer::ptr screenquadVbo;
static Vao::ptr screenquadVao;

// cache of features currently enabled
static std::set<GLenum> featureCache;

// current state
static Vao::ptr currentVao;
static GLenum   currentFaceOrder;

std::map<uint32_t, Texture::weakptr> textureCache;

compiledMesh::~compiledMesh() {
	SDL_Log("Freeing a compiledMesh");
}

compiledModel::~compiledModel() {
	SDL_Log("Freeing a compiledModel");
}

void initializeOpengl(void) {
	int maxImageUnits = 0;
	int maxCombined = 0;
	int maxVertexUniforms = 0;
	int maxFragmentUniforms = 0;
	int maxTextureSize = 0;
	int maxUBOBindings = 0;
	int maxUBOSize = 0;

	glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS,          &maxImageUnits);
	glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &maxCombined);
	glGetIntegerv(GL_MAX_VERTEX_UNIFORM_VECTORS,       &maxVertexUniforms);
	glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_VECTORS,     &maxFragmentUniforms);
	glGetIntegerv(GL_MAX_TEXTURE_SIZE,                 &maxTextureSize);

#if GLSL_VERSION >= 140 /* OpenGL 3.1+ */
	glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_BLOCKS,      &maxUBOBindings);
	glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE,           &maxUBOSize);
#endif

	SDL_Log(" OpenGL initializing... ");
	SDL_Log(" OpenGL %s", glGetString(GL_VERSION));
	SDL_Log(" OpenGL vendor: %s", glGetString(GL_VENDOR));
	SDL_Log(" OpenGL renderer: %s", glGetString(GL_RENDERER));

	SDL_Log(" OpenGL GLSL target: %d", GLSL_VERSION);
	SDL_Log(" OpenGL max fragment textures: %d", maxImageUnits);
	SDL_Log(" OpenGL max total textures: %d", maxCombined);
	SDL_Log(" OpenGL max texture size: %d", maxTextureSize);
	SDL_Log(" OpenGL max vertex vector uniforms: %d", maxVertexUniforms);
	SDL_Log(" OpenGL max fragment vector uniforms: %d", maxFragmentUniforms);
	SDL_Log(" OpenGL max uniform block bindings: %d", maxUBOBindings);
	SDL_Log(" OpenGL max uniform block size: %d", maxUBOSize);

	if (maxImageUnits < 8) {
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

Texture::ptr texcache(materialTexture::ptr tex, bool srgb) {
	if (!tex || !tex->loaded()) {
		return nullptr;
	}

	uint32_t hash = dumbhash(tex->pixels);
	auto it = textureCache.find(hash);

	if (it != textureCache.end()) {
		if (auto observe = it->second.lock()) {
			static const unsigned sizes[4] = {256, 512, 2048, 4096};
			SDL_Log("texture cache hit: %08x (%uK)\n", hash, sizes[hash >> 30]);
			return observe;
		}
	}

	Texture::ptr ret = genTexture();

	textureCache[hash] = ret;
	ret->buffer(tex, srgb);

	return ret;
}

void compileMeshes(std::string objname, meshMap& meshies) {
	for (const auto& [name, mesh] : meshies) {
		compiledMesh::ptr foo = compiledMesh::ptr(new compiledMesh());
		std::string meshname = objname + "." + name;
		SDL_Log(">>> compiling mesh %s", meshname.c_str());

		mesh->comped_mesh = foo;
		mesh->compiled = true;

		foo->elements = genBuffer(GL_ELEMENT_ARRAY_BUFFER);
		foo->elements->buffer(mesh->faces.data(),
		                      mesh->faces.size() * sizeof(GLuint));

		if (mesh->meshMaterial) {
			foo->factors = mesh->meshMaterial->factors;
			auto& maps = mesh->meshMaterial->maps;

			if (maps.diffuse && maps.diffuse->loaded()) {
				foo->textures.diffuse = texcache(maps.diffuse, true);
			}

			if (maps.metalRoughness && maps.metalRoughness->loaded()) {
				foo->textures.metalRoughness = texcache(maps.metalRoughness);
			}

			if (maps.normal && maps.normal->loaded()) {
				foo->textures.normal = texcache(maps.normal);
			}

			if (maps.ambientOcclusion && maps.ambientOcclusion->loaded()) {
				foo->textures.ambientOcclusion = texcache(maps.ambientOcclusion);
			}

			if (maps.emissive && maps.emissive->loaded()) {
				foo->textures.emissive = texcache(maps.emissive, true);
			}

			if (maps.lightmap && maps.lightmap->loaded()) {
				foo->textures.lightmap = texcache(maps.lightmap, true);
			}
		}

		cookedMeshes[meshname] = foo;
	}
}

void compileModel(std::string name, gameModel::ptr model) {
	SDL_Log(" >>> compiling %s", name.c_str());

	// TODO: might be able to clear vertex info after compiling here
	compiledModel::ptr obj = compiledModel::ptr(new compiledModel());
	model->comped_model = obj;
	model->compiled = true;

	obj->vertices = genBuffer(GL_ARRAY_BUFFER);
	obj->vertices->buffer(model->vertices.data(),
	                      model->vertices.size() * sizeof(gameModel::vertex));

	if (model->haveJoints) {
		obj->haveJoints = true;
		obj->joints = genBuffer(GL_ARRAY_BUFFER);
		obj->joints->buffer(model->joints.data(),
		                    model->joints.size() * sizeof(gameModel::jointWeights));
	}

	// collect mesh subnodes from the model
	meshMap meshes;
	for (auto& [meshname, ptr] : model->nodes) {
		if (ptr->type == gameObject::objType::Mesh) {
			// TODO: mesh naming is kind of crap
			std::string asdf = name + "." + meshname;
			SDL_Log(" > have cooked mesh %s", asdf.c_str());
			obj->meshes.push_back(asdf);
			meshes[meshname] = std::dynamic_pointer_cast<gameMesh>(ptr);
		}
	}

	compileMeshes(name, meshes);
	cookedModels[name] = obj;
	DO_ERROR_CHECK();
}

void compileModels(modelMap& models) {
	for (const auto& x : models) {
		compileModel(x.first, x.second);
	}
}

#define STRUCT_OFFSET(TYPE, MEMBER) \
	((void *)&(((TYPE*)NULL)->MEMBER))

// constexpr, should be fine right?
// otherwise can just assume 4 bytes, do sizeof()
#define GLM_VEC_ENTRIES(TYPE, MEMBER) \
	((((TYPE*)NULL)->MEMBER).length())

#define SET_VERTEX_VAO(BINDING, TYPE, MEMBER) \
	do { \
	glVertexAttribPointer(BINDING, GLM_VEC_ENTRIES(TYPE, MEMBER), \
	                      GL_FLOAT, GL_FALSE, sizeof(TYPE),       \
	                      STRUCT_OFFSET(TYPE, MEMBER));           \
	} while(0);


Vao::ptr preloadMeshVao(compiledModel::ptr obj, compiledMesh::ptr mesh) {
	if (mesh == nullptr) {
		SDL_Log("/!\\ Have broken mesh name...");
		return currentVao;
	}

	Vao::ptr orig_vao = currentVao;
	Vao::ptr ret = bindVao(genVao());

	mesh->elements->bind();
	glEnableVertexAttribArray(VAO_ELEMENTS);
	glVertexAttribPointer(VAO_ELEMENTS, 3, GL_UNSIGNED_INT, GL_FALSE, 0, 0);

	obj->vertices->bind();
	glEnableVertexAttribArray(VAO_VERTICES);
	SET_VERTEX_VAO(VAO_VERTICES, gameModel::vertex, position);

	glEnableVertexAttribArray(VAO_NORMALS);
	SET_VERTEX_VAO(VAO_NORMALS, gameModel::vertex, normal);

	glEnableVertexAttribArray(VAO_TANGENTS);
	SET_VERTEX_VAO(VAO_TANGENTS, gameModel::vertex, tangent);

	glEnableVertexAttribArray(VAO_COLORS);
	SET_VERTEX_VAO(VAO_COLORS, gameModel::vertex, color);

	glEnableVertexAttribArray(VAO_TEXCOORDS);
	SET_VERTEX_VAO(VAO_TEXCOORDS, gameModel::vertex, uv);

	glEnableVertexAttribArray(VAO_LIGHTMAP);
	SET_VERTEX_VAO(VAO_LIGHTMAP, gameModel::vertex, lightmap);

	if (obj->haveJoints) {
		obj->joints->bind();
		glEnableVertexAttribArray(VAO_JOINTS);
		SET_VERTEX_VAO(VAO_JOINTS, gameModel::jointWeights, joints);

		glEnableVertexAttribArray(VAO_JOINT_WEIGHTS);
		SET_VERTEX_VAO(VAO_JOINT_WEIGHTS, gameModel::jointWeights, weights);
	}

	bindVao(orig_vao);
	DO_ERROR_CHECK();
	return ret;
}

Vao::ptr preloadModelVao(compiledModel::ptr obj) {
	Vao::ptr orig_vao = currentVao;

	for (std::string& mesh_name : obj->meshes) {
		if (auto ptr = cookedMeshes[mesh_name].lock()) {
			SDL_Log(" # binding mesh %s", mesh_name.c_str());
			ptr->vao = preloadMeshVao(obj, ptr);
		}
	}

	return orig_vao;
}

void bindModel(gameModel::ptr model) {
	if (model->comped_model == nullptr) {
		SDL_Log(" # ERROR: trying to bind an uncompiled model");
	}

	SDL_Log(" # binding a model");
	model->comped_model->vao = preloadModelVao(model->comped_model);
}

void bindCookedMeshes(void) {
	for (auto& [name, wptr] : cookedModels) {
		if (auto ptr = wptr.lock()) {
			SDL_Log(" # binding %s", name.c_str());
			ptr->vao = preloadModelVao(ptr);
		}
	}
}

static std::vector<GLfloat> screenquad_data = {
	-1, -1,  0, 0, 0,
	 1, -1,  0, 1, 0,
	 1,  1,  0, 1, 1,

	 1,  1,  0, 1, 1,
	-1,  1,  0, 0, 1,
	-1, -1,  0, 0, 0,
};

void preloadScreenquad(void) {
	screenquadVbo = genBuffer(GL_ARRAY_BUFFER);
	Vao::ptr orig_vao = currentVao;
	screenquadVao = bindVao(genVao());

	screenquadVbo->buffer(screenquad_data);
	glEnableVertexAttribArray(VAO_QUAD_VERTICES);
	glVertexAttribPointer(VAO_QUAD_VERTICES,
	                      3, GL_FLOAT, GL_FALSE, 5*sizeof(float), 0);
	glEnableVertexAttribArray(VAO_QUAD_TEXCOORDS);
	glVertexAttribPointer(VAO_QUAD_TEXCOORDS,
	                      2, GL_FLOAT, GL_FALSE, 5*sizeof(float),
	                      (void*)(3 * sizeof(float)));

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

		SDL_Log("/!\\ OPENGL ERROR: %s:%d: %s(): %s (errno. %d)",
			filename, line, func, errstr, err);

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

Texture::ptr genTextureColor(unsigned width, unsigned height, GLenum format) {
	Texture::ptr ret = genTexture();

	glBindTexture(GL_TEXTURE_2D, ret->obj);
	glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0,
	             GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	return ret;
}

Texture::ptr
genTextureDepthStencil(unsigned width, unsigned height, GLenum format) {
	Texture::ptr ret = genTexture();

	glBindTexture(GL_TEXTURE_2D, ret->obj);
	glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0,
	             GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, nullptr);
	DO_ERROR_CHECK();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	return ret;
}

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

GLenum surfaceGlFormat(const materialTexture& tex) {
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

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, 0);
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
