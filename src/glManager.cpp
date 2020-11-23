#include <grend/glManager.hpp>
#include <grend/gameModel.hpp>

#include <string>
#include <vector>
#include <map>

#include <fstream>
#include <iostream>
#include <sstream>

namespace grendx {

static cookedMeshMap  cookedMeshes;
static cookedModelMap cookedModels;

static std::vector<GLfloat> cookedJoints;
static std::vector<GLfloat> cookedVertprops;
static std::vector<GLuint> cookedElements;

static Buffer::ptr cookedVertprops_vbo;
static Buffer::ptr cookedElementVbo;
static Buffer::ptr cookedJoints_vbo;
static Buffer::ptr screenquadVbo;
static Vao::ptr screenquadVao;

static bufferAllocator vertpropsAlloc;
static bufferAllocator elementsAlloc;
static bufferAllocator jointsAlloc;

// cache of features currently enabled
static std::set<GLenum> featureCache;

// current state
static Vao::ptr currentVao;
static GLenum   currentFaceOrder;

std::map<uint32_t, Texture::weakptr> textureCache;

compiledMesh::~compiledMesh() {
	std::cerr << "Freeing a compiledMesh" << std::endl;
	elementsAlloc.free(elements);
}

compiledModel::~compiledModel() {
	std::cerr << "Freeing a compiledModel" << std::endl;
	vertpropsAlloc.free(vertices);
	if (haveJoints) {
		jointsAlloc.free(joints);
	}
}

void initializeOpengl(void) {
	std::cerr << " # Got here, " << __func__ << std::endl;
	std::cerr << " # maximum combined texture units: " << GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS << std::endl;

	// make sure we start with a bound VAO
	bindVao(genVao());

	// initialize state for quad that fills the screen
	preloadScreenquad();

	cookedVertprops.clear();
	cookedElements.clear();

	cookedVertprops_vbo = genBuffer(GL_ARRAY_BUFFER);
	cookedElementVbo = genBuffer(GL_ELEMENT_ARRAY_BUFFER);
	cookedJoints_vbo = genBuffer(GL_ARRAY_BUFFER);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void compileMeshes(std::string objname, meshMap& meshies) {
	for (const auto& [name, mesh] : meshies) {
		compiledMesh::ptr foo = compiledMesh::ptr(new compiledMesh());
		std::string meshname = objname + "." + name;
		std::cerr << ">>> compiling mesh " << meshname << std::endl;

		mesh->comped_mesh = foo;
		mesh->compiled = true;

		//foo.elementsSize = x.second.faces.size() * sizeof(GLushort);
		/*
		foo->elementsSize = mesh->faces.size();
		foo->elementsOffset = reinterpret_cast<void*>
		                      (cookedElements.size() * sizeof(GLuint));
							  */
		/*
		cookedElements.insert(cookedElements.end(), mesh->faces.begin(),
		                                            mesh->faces.end());
													*/

		foo->elements = elementsAlloc.allocate(mesh->faces.size() * sizeof(GLuint));
		assert(foo->elements != nullptr);
		cookedElements.reserve((foo->elements->size + foo->elements->offset) / sizeof(GLuint));
		cookedElements.resize(cookedElements.capacity());

		size_t bufoffset = foo->elements->offset / sizeof(GLuint);
		for (size_t i = 0; i < mesh->faces.size(); i++) {
			cookedElements[bufoffset + i] = mesh->faces[i];
		}

		foo->material = mesh->material;
		cookedMeshes[meshname] = foo;
	}
	//fprintf(stderr, " > elements size %lu\n", cookedElements.size());
}

// TODO: move
static const size_t VERTPROP_SIZE = (sizeof(glm::vec3[3]) + sizeof(glm::vec2));
static const size_t JOINTPROP_SIZE = (sizeof(glm::vec4[2]));

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

Texture::ptr texcache(const materialTexture& tex, bool srgb) {
	if (!tex.loaded()) {
		return nullptr;
	}

	uint32_t hash = dumbhash(tex.pixels);
	auto it = textureCache.find(hash);

	if (it != textureCache.end()) {
		if (auto observe = it->second.lock()) {
			static const unsigned sizes[4] = {256, 512, 2048, 4096};
			fprintf(stderr, "texture cache hit: %08x (%uK)\n",
			        hash, sizes[hash >> 30]);
			return observe;
		}
	}

	Texture::ptr ret = genTexture();

	textureCache[hash] = ret;
	ret->buffer(tex, srgb);

	return ret;
}

void compileModel(std::string name, gameModel::ptr model) {
	std::cerr << " >>> compiling " << name << std::endl;

	assert(model->vertices.size() == model->normals.size());
	assert(model->vertices.size() == model->texcoords.size());
	//assert(model->vertices.size() == model->tangents.size());
	//assert(model->vertices.size() == model->bitangents.size());

	compiledModel::ptr obj = compiledModel::ptr(new compiledModel());
	model->comped_model = obj;
	model->compiled = true;
	// TODO: might be able to clear vertex info after compiling here
	/*
	obj->verticesSize = model->vertices.size() * VERTPROP_SIZE;

	size_t offset = cookedVertprops.size() * sizeof(GLfloat);

	auto offset_ptr = [] (uintptr_t off) {
		return reinterpret_cast<void*>(off);
	};

	obj->verticesOffset   = offset_ptr(offset);
	obj->normalsOffset    = offset_ptr(offset + sizeof(glm::vec3[1]));
	obj->tangentsOffset   = offset_ptr(offset + sizeof(glm::vec3[2]));
	obj->texcoordsOffset  = offset_ptr(offset + sizeof(glm::vec3[3]));
	*/

	obj->vertices = vertpropsAlloc.allocate(model->vertices.size() * VERTPROP_SIZE);
	cookedVertprops.reserve((obj->vertices->size + obj->vertices->offset) / sizeof(GLfloat));
	cookedVertprops.resize(cookedVertprops.capacity());

	// XXX: glm vectors don't have an iterator
	auto set_vec = [&] (size_t *off, auto& props, const auto& vec) {
		for (size_t k = 0; k < vec.length(); k++) {
			props[*off + k] = vec[k];
		}
		(*off) += vec.length();
	};

	size_t offset = obj->vertices->offset / sizeof(GLfloat);
	for (size_t i = 0; i < model->vertices.size(); i++) {
		set_vec(&offset, cookedVertprops, model->vertices[i]);
		set_vec(&offset, cookedVertprops, model->normals[i]);
		set_vec(&offset, cookedVertprops, model->tangents[i]);
		set_vec(&offset, cookedVertprops, model->texcoords[i]);
	}

	if (model->haveJoints) {
		std::cerr << " > have joints, " << model->joints.size()
			<< " of em" << std::endl;
		/*
		size_t jointoff = cookedJoints.size() * sizeof(GLfloat);

		obj->haveJoints = true;
		obj->jointsOffset  = offset_ptr(jointoff);
		obj->weightsOffset = offset_ptr(jointoff + sizeof(glm::vec4[1]));
		*/

		obj->haveJoints = true;
		obj->joints = jointsAlloc.allocate(model->joints.size() * JOINTPROP_SIZE);
		cookedJoints.reserve((obj->joints->size + obj->joints->offset) / (sizeof(GLfloat)));
		cookedJoints.resize(cookedJoints.capacity());

		size_t offset = obj->joints->offset / sizeof(GLfloat);
		for (unsigned i = 0; i < model->joints.size(); i++) {
			set_vec(&offset, cookedJoints, model->joints[i]);
			set_vec(&offset, cookedJoints, model->weights[i]);
		}
	}

	// collect mesh subnodes from the model
	meshMap meshes;
	for (auto& [meshname, ptr] : model->nodes) {
		if (ptr->type == gameObject::objType::Mesh) {
			// TODO: mesh naming is kind of crap
			std::string asdf = name + "." + meshname;
			std::cerr << " > have cooked mesh " << asdf << std::endl;
			//obj->meshes.push_back(asdf);
			obj->meshes.push_back(asdf);
			meshes[meshname] = std::dynamic_pointer_cast<gameMesh>(ptr);
		}
	}

	//compileMeshes(name, model->meshes);
	compileMeshes(name, meshes);

	/*
	// copy mesh names
	for (const auto& [name, ptr] : meshes) {
		std::string asdf = name + "." + name;
		std::cerr << " > have cooked mesh " << asdf << std::endl;
		obj->meshes.push_back(asdf);
	}
	*/

	// copy materials
	for (const auto& [name, mat] : model->materials) {
		//obj->materials[name] = mat;
		obj->materials[name].copy_properties(mat);

		if (mat.diffuseMap.loaded()) {
			obj->matTextures[name] = texcache(mat.diffuseMap, true/*srgb*/);
		}

		if (mat.metalRoughnessMap.loaded()) {
			obj->matSpecular[name] = texcache(mat.metalRoughnessMap);
		}

		if (mat.normalMap.loaded()) {
			obj->matNormal[name]   = texcache(mat.normalMap);
		}

		if (mat.ambientOcclusionMap.loaded()) {
			obj->matAo[name]       = texcache(mat.ambientOcclusionMap);
		}

		if (mat.emissiveMap.loaded()) {
			// TODO: clearer sRGB flag
			obj->matEmissive[name] = texcache(mat.emissiveMap, true /* srgb */);
		}
	}

	cookedModels[name] = obj;
}

void compileModels(modelMap& models) {
	for (const auto& x : models) {
		compileModel(x.first, x.second);
	}
}

Vao::ptr preloadMeshVao(compiledModel::ptr obj, compiledMesh::ptr mesh) {
	if (mesh == nullptr) {
		std::cerr << "/!\\ Have broken mesh name..." << std::endl;
		return currentVao;
	}

	Vao::ptr orig_vao = currentVao;
	Vao::ptr ret = bindVao(genVao());

	//bind_vbo(cookedElementVbo, GL_ELEMENT_ARRAY_BUFFER);
	cookedElementVbo->bind();
	glEnableVertexAttribArray(VAO_ELEMENTS);
	glVertexAttribPointer(VAO_ELEMENTS, 3, GL_UNSIGNED_INT, GL_FALSE, 0,
	                      (void*)(mesh->elements->offset));

	cookedVertprops_vbo->bind();
	glEnableVertexAttribArray(VAO_VERTICES);
	glVertexAttribPointer(VAO_VERTICES, 3, GL_FLOAT, GL_FALSE, VERTPROP_SIZE,
	                      (void*)(obj->vertices->offset));

	glEnableVertexAttribArray(VAO_NORMALS);
	glVertexAttribPointer(VAO_NORMALS, 3, GL_FLOAT, GL_FALSE, VERTPROP_SIZE,
	                      (void*)(obj->vertices->offset + sizeof(glm::vec3[1])));

	glEnableVertexAttribArray(VAO_TANGENTS);
	glVertexAttribPointer(VAO_TANGENTS, 3, GL_FLOAT, GL_FALSE, VERTPROP_SIZE,
	                      (void*)(obj->vertices->offset + sizeof(glm::vec3[2])));

	glEnableVertexAttribArray(VAO_TEXCOORDS);
	glVertexAttribPointer(VAO_TEXCOORDS, 2, GL_FLOAT, GL_FALSE, VERTPROP_SIZE,
	                      (void*)(obj->vertices->offset + sizeof(glm::vec3[3])));

	//bind_vbo(cookedVertprops_vbo, GL_ARRAY_BUFFER);
	/*
	glEnableVertexAttribArray(VAO_VERTICES);
	glVertexAttribPointer(VAO_VERTICES, 3, GL_FLOAT, GL_FALSE, VERTPROP_SIZE,
	                      obj->verticesOffset);

	glEnableVertexAttribArray(VAO_NORMALS);
	glVertexAttribPointer(VAO_NORMALS, 3, GL_FLOAT, GL_FALSE, VERTPROP_SIZE,
	                      obj->normalsOffset);

	glEnableVertexAttribArray(VAO_TANGENTS);
	glVertexAttribPointer(VAO_TANGENTS, 3, GL_FLOAT, GL_FALSE, VERTPROP_SIZE,
	                      obj->tangentsOffset);

	glEnableVertexAttribArray(VAO_TEXCOORDS);
	glVertexAttribPointer(VAO_TEXCOORDS, 2, GL_FLOAT, GL_FALSE, VERTPROP_SIZE,
	                      obj->texcoordsOffset);
						  */

	if (obj->haveJoints) {
		cookedJoints_vbo->bind();
		glEnableVertexAttribArray(VAO_JOINTS);
		glVertexAttribPointer(VAO_JOINTS, 4, GL_FLOAT, GL_FALSE, JOINTPROP_SIZE,
							  (void*)(obj->joints->offset));
		glEnableVertexAttribArray(VAO_JOINT_WEIGHTS);
		glVertexAttribPointer(VAO_JOINT_WEIGHTS,
		                      4, GL_FLOAT, GL_FALSE, JOINTPROP_SIZE,
							  (void*)(obj->joints->offset + sizeof(glm::vec4)));
		/*
		glEnableVertexAttribArray(VAO_JOINTS);
		glVertexAttribPointer(VAO_JOINTS, 4, GL_FLOAT, GL_FALSE, JOINTPROP_SIZE,
							  obj->jointsOffset);
		glEnableVertexAttribArray(VAO_JOINT_WEIGHTS);
		glVertexAttribPointer(VAO_JOINT_WEIGHTS,
		                      4, GL_FLOAT, GL_FALSE, JOINTPROP_SIZE,
							  obj->weightsOffset);
							  */
	}

	bindVao(orig_vao);
	return ret;
}

Vao::ptr preloadModelVao(compiledModel::ptr obj) {
	Vao::ptr orig_vao = currentVao;

	for (std::string& mesh_name : obj->meshes) {
		if (auto ptr = cookedMeshes[mesh_name].lock()) {
			std::cerr << " # binding mesh " << mesh_name << std::endl;
			ptr->vao = preloadMeshVao(obj, ptr);
		}
	}

	return orig_vao;
}

void bindCookedMeshes(void) {
	std::cerr << "bindCookedMeshes(): "
		<< "vertprops: " << (cookedVertprops.size()*sizeof(GLfloat)/1024.f) << "kb, "
		<< "elements: " << (cookedElements.size()*sizeof(GLuint)/1024.f) << "kb, "
		<< "joints: " << (cookedJoints.size()*sizeof(GLuint)/1024.f) << "kb"
		<< std::endl;
	std::cerr << "bindCookedMeshes(): capacity: "
		<< "vertprops: " << (cookedVertprops.capacity()*sizeof(GLfloat)/1024.f) << "kb, "
		<< "elements: " << (cookedElements.capacity()*sizeof(GLuint)/1024.f) << "kb, "
		<< "joints: " << (cookedJoints.capacity()*sizeof(GLuint)/1024.f) << "kb"
		<< std::endl;


	cookedJoints_vbo->buffer(cookedJoints);
	cookedVertprops_vbo->buffer(cookedVertprops);
	cookedElementVbo->buffer(cookedElements);

	for (auto& [name, wptr] : cookedModels) {
		if (auto ptr = wptr.lock()) {
			std::cerr << " # binding " << name << std::endl;
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
		std::cerr << "/!\\ ERROR: " << filename << ":" << line
			<< ": " << func << "(): ";

		switch (err) {
			case GL_INVALID_ENUM:     
				std::cerr << "invalid enum"; break;
			case GL_INVALID_VALUE:    
				std::cerr << "invalid value"; break;
			case GL_INVALID_OPERATION:
				std::cerr << "invalid operation"; break;
			case GL_INVALID_FRAMEBUFFER_OPERATION:
				std::cerr << "invalid framebuffer operation"; break;
			case GL_OUT_OF_MEMORY:
				std::cerr << "out of memory"; break;
			case GL_STACK_UNDERFLOW:
				std::cerr << "stack underflow"; break;
			case GL_STACK_OVERFLOW:
				std::cerr << "stack overflow"; break;
			default:
				std::cerr << "some kind of error code, #" << err; break;
		}

		std::cerr << std::endl;
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
	disable(GL_CULL_FACE);
	glStencilMask(~0);
	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
	glFrontFace(GL_CCW);
	glCullFace(GL_BACK);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glStencilFunc(GL_ALWAYS, 0, ~0);

	disable(GL_SCISSOR_TEST);
	enable(GL_BLEND);
	enable(GL_DEPTH_TEST);
	enable(GL_CULL_FACE);;
	// TODO: other flags

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindVertexArray(0);
}

void enable(GLenum feature) {
	if (featureCache.find(feature) == featureCache.end()) {
		glEnable(feature);
		featureCache.insert(feature);
	}
}

void disable(GLenum feature) {
	auto it = featureCache.find(feature);

	if (it != featureCache.end()) {
		glDisable(feature);
		featureCache.erase(it);
	}
}

// namespace grendx
}
