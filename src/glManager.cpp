#include <grend/glManager.hpp>
#include <grend/sceneModel.hpp>
#include <SDL.h>

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
static std::map<material*, compiledMaterial::weakptr> materialCache;

compiledMaterial::~compiledMaterial() {
	//SDL_Log("Freeing a compiledMaterial");
}

compiledMesh::~compiledMesh() {
	//SDL_Log("Freeing a compiledMesh");
}

compiledModel::~compiledModel() {
	//SDL_Log("Freeing a compiledModel");
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

	glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS,          &maxImageUnits);
	glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &maxCombined);
	glGetIntegerv(GL_MAX_VERTEX_UNIFORM_VECTORS,       &maxVertexUniforms);
	glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_VECTORS,     &maxFragmentUniforms);
	glGetIntegerv(GL_MAX_TEXTURE_SIZE,                 &maxTextureSize);

#if GLSL_VERSION >= 140 /* OpenGL 3.1+ */
	glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS,            &maxFramebufAttachments);
	glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_BLOCKS,      &maxUBOBindings);
	glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE,           &maxUBOSize);
#endif
	DO_ERROR_CHECK();

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
	SDL_Log(" OpenGL max framebuffer attachments: %u", maxFramebufAttachments);

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

Texture::ptr texcache(materialTexture::ptr tex, bool srgb) {
	if (!tex || !tex->loaded()) {
		return nullptr;
	}

	uint32_t hash = dumbhash(tex->pixels);
	auto it = textureCache.find(hash);

	if (it != textureCache.end()) {
		if (auto observe = it->second.lock()) {
			//static const unsigned sizes[4] = {256, 512, 2048, 4096};
			//SDL_Log("texture cache hit: %08x (%uK)\n", hash, sizes[hash >> 30]);
			return observe;
		}
	}

	Texture::ptr ret = genTexture();

	textureCache[hash] = ret;
	ret->buffer(tex, srgb);

	return ret;
}

compiledMaterial::ptr matcache(material::ptr mat) {
	if (!mat) {
		return nullptr;
	}

	auto it = materialCache.find(mat.get());

	if (it != materialCache.end()) {
		if (auto observe = it->second.lock()) {
			return observe;
		}
	}

	compiledMaterial::ptr ret = std::make_shared<compiledMaterial>();

	ret->factors = mat->factors;
	auto& maps = mat->maps;

	if (maps.diffuse && maps.diffuse->loaded()) {
		ret->textures.diffuse = texcache(maps.diffuse, true);
	}

	if (maps.metalRoughness && maps.metalRoughness->loaded()) {
		ret->textures.metalRoughness = texcache(maps.metalRoughness);
	}

	if (maps.normal && maps.normal->loaded()) {
		ret->textures.normal = texcache(maps.normal);
	}

	if (maps.ambientOcclusion && maps.ambientOcclusion->loaded()) {
		ret->textures.ambientOcclusion = texcache(maps.ambientOcclusion);
	}

	if (maps.emissive && maps.emissive->loaded()) {
		ret->textures.emissive = texcache(maps.emissive, true);
	}

	if (maps.lightmap && maps.lightmap->loaded()) {
		ret->textures.lightmap = texcache(maps.lightmap, true);
	}

	materialCache[mat.get()] = ret;
	// XXX: once the material is cached no need to keep pointers around...
	//      need better way to do this
	maps.diffuse.reset();
	maps.metalRoughness.reset();
	maps.normal.reset();
	maps.ambientOcclusion.reset();
	maps.emissive.reset();
	maps.lightmap.reset();

	return ret;
}

compiledMesh::ptr compileMesh(sceneMesh::ptr& mesh) {
	compiledMesh::ptr foo = compiledMesh::ptr(new compiledMesh());

	if (mesh->faces.size() == 0) {
		SDL_Log("Mesh has no indices!\n");
		return foo;
	}

	mesh->comped_mesh = foo;
	mesh->compiled = true;

	foo->elements = genBuffer(GL_ELEMENT_ARRAY_BUFFER);
	foo->elements->buffer(mesh->faces.data(),
						  mesh->faces.size() * sizeof(GLuint));

	// TODO: more consistent naming here
	foo->mat   = matcache(mesh->meshMaterial);
	foo->blend = foo->mat? foo->mat->factors.blend : material::blend_mode::Opaque;

	return foo;
}

compiledModel::ptr compileModel(std::string name, sceneModel::ptr model) {
	// TODO: might be able to clear vertex info after compiling here
	compiledModel::ptr obj = compiledModel::ptr(new compiledModel());
	model->comped_model = obj;
	model->compiled = true;

	obj->vertices = genBuffer(GL_ARRAY_BUFFER);
	obj->vertices->buffer(model->vertices.data(),
	                      model->vertices.size() * sizeof(sceneModel::vertex));

	if (model->haveJoints) {
		obj->haveJoints = true;
		obj->joints = genBuffer(GL_ARRAY_BUFFER);
		obj->joints->buffer(model->joints.data(),
		                    model->joints.size() * sizeof(sceneModel::jointWeights));
	}

	for (auto& [meshname, ptr] : model->nodes) {
		if (ptr->type == sceneNode::objType::Mesh) {
			auto wptr = std::dynamic_pointer_cast<sceneMesh>(ptr);
			obj->meshes[meshname] = compileMesh(wptr);
		}
	}

	DO_ERROR_CHECK();
	obj->vao = preloadModelVao(obj);

	return obj;
}

void compileModels(const modelMap& models) {
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

#define SET_VAO_ENTRY(BINDING, TYPE, MEMBER) \
	do { \
	glVertexAttribPointer(BINDING, GLM_VEC_ENTRIES(TYPE, MEMBER), \
	                      GL_FLOAT, GL_FALSE, sizeof(TYPE),       \
	                      STRUCT_OFFSET(TYPE, MEMBER));           \
	} while(0);


Vao::ptr preloadMeshVao(compiledModel::ptr obj, compiledMesh::ptr mesh) {
	if (mesh == nullptr || !mesh->elements) {
		SDL_Log("/!\\ Have broken mesh...");
		return currentVao;
	}

	Vao::ptr orig_vao = currentVao;
	Vao::ptr ret = bindVao(genVao());

	mesh->elements->bind();
	glEnableVertexAttribArray(VAO_ELEMENTS);
	glVertexAttribPointer(VAO_ELEMENTS, 3, GL_UNSIGNED_INT, GL_FALSE, 0, 0);

	obj->vertices->bind();
	glEnableVertexAttribArray(VAO_VERTICES);
	SET_VAO_ENTRY(VAO_VERTICES, sceneModel::vertex, position);

	glEnableVertexAttribArray(VAO_NORMALS);
	SET_VAO_ENTRY(VAO_NORMALS, sceneModel::vertex, normal);

	glEnableVertexAttribArray(VAO_TANGENTS);
	SET_VAO_ENTRY(VAO_TANGENTS, sceneModel::vertex, tangent);

	glEnableVertexAttribArray(VAO_COLORS);
	SET_VAO_ENTRY(VAO_COLORS, sceneModel::vertex, color);

	glEnableVertexAttribArray(VAO_TEXCOORDS);
	SET_VAO_ENTRY(VAO_TEXCOORDS, sceneModel::vertex, uv);

	glEnableVertexAttribArray(VAO_LIGHTMAP);
	SET_VAO_ENTRY(VAO_LIGHTMAP, sceneModel::vertex, lightmap);

	if (obj->haveJoints) {
		obj->joints->bind();
		glEnableVertexAttribArray(VAO_JOINTS);
		SET_VAO_ENTRY(VAO_JOINTS, sceneModel::jointWeights, joints);

		glEnableVertexAttribArray(VAO_JOINT_WEIGHTS);
		SET_VAO_ENTRY(VAO_JOINT_WEIGHTS, sceneModel::jointWeights, weights);
	}

	bindVao(orig_vao);
	DO_ERROR_CHECK();
	return ret;
}

Vao::ptr preloadModelVao(compiledModel::ptr obj) {
	Vao::ptr orig_vao = currentVao;

	for (auto [mesh_name, ptr] : obj->meshes) {
		ptr->vao = preloadMeshVao(obj, ptr);
	}

	return orig_vao;
}

void bindModel(sceneModel::ptr model) {
	if (model->comped_model == nullptr) {
		SDL_Log(" # ERROR: trying to bind an uncompiled model");
	}

	model->comped_model->vao = preloadModelVao(model->comped_model);
}

// TODO: now that things are cleaned up a lot, this function isn't necessary
//       at all, just call bindModel at the end of the compile call...
void bindCookedMeshes(void) {
	/*
	SDL_Log(" # rebinding all models...");

	for (auto& wptr : cookedModels) {
		if (auto ptr = wptr.lock()) {
			ptr->vao = preloadModelVao(ptr);
		}
	}
	*/
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
