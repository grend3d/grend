#include <grend/gl_manager.hpp>
#include <grend/gameModel.hpp>

#include <string>
#include <vector>
#include <map>

#include <fstream>
#include <iostream>
#include <sstream>

namespace grendx {

static cooked_mesh_map  cooked_meshes;
static cooked_model_map cooked_models;

static std::vector<GLfloat> cooked_joints;
static std::vector<GLfloat> cooked_vertprops;
static std::vector<GLuint> cooked_elements;

static Vbo::ptr cooked_vertprops_vbo;
static Vbo::ptr cooked_element_vbo;
static Vbo::ptr cooked_joints_vbo;
static Vbo::ptr screenquad_vbo;
static Vao::ptr screenquad_vao;

// cache of features currently enabled
static std::set<GLenum> feature_cache;

// current state
static Vao::ptr current_vao;
static GLenum   current_face_order;

std::map<uint32_t, Texture::weakptr> texture_cache;

void initialize_opengl(void) {
	std::cerr << " # Got here, " << __func__ << std::endl;
	std::cerr << " # maximum combined texture units: " << GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS << std::endl;

	// make sure we start with a bound VAO
	bind_vao(gen_vao());

	// initialize state for quad that fills the screen
	preload_screenquad();

	cooked_vertprops.clear();
	cooked_elements.clear();

	cooked_vertprops_vbo = gen_vbo();
	cooked_element_vbo = gen_vbo();
	cooked_joints_vbo = gen_vbo();

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void compile_meshes(std::string objname, mesh_map& meshies) {
	for (const auto& [name, mesh] : meshies) {
		compiled_mesh::ptr foo = compiled_mesh::ptr(new compiled_mesh());
		std::string meshname = objname + "." + name;
		std::cerr << ">>> compiling mesh " << meshname << std::endl;

		mesh->comped_mesh = foo;
		mesh->compiled = true;

		//foo.elements_size = x.second.faces.size() * sizeof(GLushort);
		foo->elements_size = mesh->faces.size();
		foo->elements_offset = reinterpret_cast<void*>
		                      (cooked_elements.size() * sizeof(GLuint));
		cooked_elements.insert(cooked_elements.end(), mesh->faces.begin(),
		                                              mesh->faces.end());
		foo->material = mesh->material;
		cooked_meshes[meshname] = foo;
	}
	//fprintf(stderr, " > elements size %lu\n", cooked_elements.size());
}

// TODO: move
static const size_t VERTPROP_SIZE = (sizeof(glm::vec3[4]) + sizeof(glm::vec2));
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
	auto it = texture_cache.find(hash);

	if (it != texture_cache.end()) {
		if (auto observe = it->second.lock()) {
			static const unsigned sizes[4] = {256, 512, 2048, 4096};
			fprintf(stderr, "texture cache hit: %08x (%uK)\n",
			        hash, sizes[hash >> 30]);
			return observe;
		}
	}

	Texture::ptr ret = gen_texture();

	texture_cache[hash] = ret;
	ret->buffer(tex, srgb);

	return ret;
}

void compile_model(std::string name, gameModel::ptr model) {
	std::cerr << " >>> compiling " << name << std::endl;

	assert(model->vertices.size() == model->normals.size());
	assert(model->vertices.size() == model->texcoords.size());
	//assert(model->vertices.size() == model->tangents.size());
	//assert(model->vertices.size() == model->bitangents.size());

	compiled_model::ptr obj = compiled_model::ptr(new compiled_model());
	model->comped_model = obj;
	model->compiled = true;
	// TODO: might be able to clear vertex info after compiling here
	obj->vertices_size = model->vertices.size() * VERTPROP_SIZE;

	size_t offset = cooked_vertprops.size() * sizeof(GLfloat);

	auto offset_ptr = [] (uintptr_t off) {
		return reinterpret_cast<void*>(off);
	};

	obj->vertices_offset   = offset_ptr(offset);
	obj->normals_offset    = offset_ptr(offset + sizeof(glm::vec3[1]));
	obj->tangents_offset   = offset_ptr(offset + sizeof(glm::vec3[2]));
	obj->bitangents_offset = offset_ptr(offset + sizeof(glm::vec3[3]));
	obj->texcoords_offset  = offset_ptr(offset + sizeof(glm::vec3[4]));

	// XXX: glm vectors don't have an iterator
	auto push_vec = [&] (auto& props, const auto& vec) {
		for (int k = 0; k < vec.length(); k++) {
			props.push_back(vec[k]);
		}
	};

	for (unsigned i = 0; i < model->vertices.size(); i++) {
		push_vec(cooked_vertprops, model->vertices[i]);
		push_vec(cooked_vertprops, model->normals[i]);
		push_vec(cooked_vertprops, model->tangents[i]);
		push_vec(cooked_vertprops, model->bitangents[i]);
		push_vec(cooked_vertprops, model->texcoords[i]);
	}

	if (model->haveJoints) {
		std::cerr << " > have joints, " << model->joints.size()
			<< " of em" << std::endl;
		size_t jointoff = cooked_joints.size() * sizeof(GLfloat);

		obj->haveJoints = true;
		obj->joints_offset  = offset_ptr(jointoff);
		obj->weights_offset = offset_ptr(jointoff + sizeof(glm::vec4[1]));

		for (unsigned i = 0; i < model->joints.size(); i++) {
			push_vec(cooked_joints, model->joints[i]);
			push_vec(cooked_joints, model->weights[i]);
		}
	}

	// collect mesh subnodes from the model
	mesh_map meshes;
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

	//compile_meshes(name, model->meshes);
	compile_meshes(name, meshes);

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

		if (mat.diffuse_map.loaded()) {
			obj->mat_textures[name] = texcache(mat.diffuse_map, true/*srgb*/);
		}

		if (mat.metal_roughness_map.loaded()) {
			obj->mat_specular[name] = texcache(mat.metal_roughness_map);
		}

		if (mat.normal_map.loaded()) {
			obj->mat_normal[name]   = texcache(mat.normal_map);
		}

		if (mat.ambient_occ_map.loaded()) {
			obj->mat_ao[name]       = texcache(mat.ambient_occ_map);
		}
	}

	cooked_models[name] = obj;
}

void compile_models(model_map& models) {
	for (const auto& x : models) {
		compile_model(x.first, x.second);
	}
}

Vao::ptr preload_mesh_vao(compiled_model::ptr obj, compiled_mesh::ptr mesh) {
	if (mesh == nullptr) {
		std::cerr << "/!\\ Have broken mesh name..." << std::endl;
		return current_vao;
	}

	Vao::ptr orig_vao = current_vao;
	Vao::ptr ret = bind_vao(gen_vao());

	//bind_vbo(cooked_element_vbo, GL_ELEMENT_ARRAY_BUFFER);
	cooked_element_vbo->bind(GL_ELEMENT_ARRAY_BUFFER);
	glEnableVertexAttribArray(VAO_ELEMENTS);
	glVertexAttribPointer(VAO_ELEMENTS, 3, GL_UNSIGNED_INT, GL_FALSE, 0,
	                      mesh->elements_offset);

	cooked_vertprops_vbo->bind(GL_ARRAY_BUFFER);
	//bind_vbo(cooked_vertprops_vbo, GL_ARRAY_BUFFER);
	glEnableVertexAttribArray(VAO_VERTICES);
	glVertexAttribPointer(VAO_VERTICES, 3, GL_FLOAT, GL_FALSE, VERTPROP_SIZE,
	                      obj->vertices_offset);

	glEnableVertexAttribArray(VAO_NORMALS);
	glVertexAttribPointer(VAO_NORMALS, 3, GL_FLOAT, GL_FALSE, VERTPROP_SIZE,
	                      obj->normals_offset);

	glEnableVertexAttribArray(VAO_TANGENTS);
	glVertexAttribPointer(VAO_TANGENTS, 3, GL_FLOAT, GL_FALSE, VERTPROP_SIZE,
	                      obj->tangents_offset);

	glEnableVertexAttribArray(VAO_BITANGENTS);
	glVertexAttribPointer(VAO_BITANGENTS, 3, GL_FLOAT, GL_FALSE, VERTPROP_SIZE,
	                      obj->bitangents_offset);

	glEnableVertexAttribArray(VAO_TEXCOORDS);
	glVertexAttribPointer(VAO_TEXCOORDS, 2, GL_FLOAT, GL_FALSE, VERTPROP_SIZE,
	                      obj->texcoords_offset);

	if (obj->haveJoints) {
		cooked_joints_vbo->bind(GL_ARRAY_BUFFER);
		glEnableVertexAttribArray(VAO_JOINTS);
		glVertexAttribPointer(VAO_JOINTS, 4, GL_FLOAT, GL_FALSE, JOINTPROP_SIZE,
							  obj->joints_offset);
		glEnableVertexAttribArray(VAO_JOINT_WEIGHTS);
		glVertexAttribPointer(VAO_JOINT_WEIGHTS,
		                      4, GL_FLOAT, GL_FALSE, JOINTPROP_SIZE,
							  obj->weights_offset);
	}

	bind_vao(orig_vao);
	return ret;
}

Vao::ptr preload_model_vao(compiled_model::ptr obj) {
	Vao::ptr orig_vao = current_vao;

	for (std::string& mesh_name : obj->meshes) {
		std::cerr << " # binding mesh " << mesh_name << std::endl;
		cooked_meshes[mesh_name]->vao
			= preload_mesh_vao(obj, cooked_meshes[mesh_name]);
	}

	return orig_vao;
}

void bind_cooked_meshes(void) {
	cooked_joints_vbo->buffer(GL_ARRAY_BUFFER, cooked_joints);
	cooked_vertprops_vbo->buffer(GL_ARRAY_BUFFER, cooked_vertprops);
	cooked_element_vbo->buffer(GL_ELEMENT_ARRAY_BUFFER, cooked_elements);

	for (auto& [name, ptr] : cooked_models) {
		std::cerr << " # binding " << name << std::endl;
		ptr->vao = preload_model_vao(ptr);
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

void preload_screenquad(void) {
	screenquad_vbo = gen_vbo();
	Vao::ptr orig_vao = current_vao;
	screenquad_vao = bind_vao(gen_vao());

	screenquad_vbo->buffer(GL_ARRAY_BUFFER, screenquad_data);
	glEnableVertexAttribArray(VAO_QUAD_VERTICES);
	glVertexAttribPointer(VAO_QUAD_VERTICES,
	                      3, GL_FLOAT, GL_FALSE, 5*sizeof(float), 0);
	glEnableVertexAttribArray(VAO_QUAD_TEXCOORDS);
	glVertexAttribPointer(VAO_QUAD_TEXCOORDS,
	                      2, GL_FLOAT, GL_FALSE, 5*sizeof(float),
	                      (void*)(3 * sizeof(float)));

	bind_vao(orig_vao);
}

void draw_screenquad(void) {
	bind_vao(get_screenquad_vao());
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

Vao::ptr gen_vao(void) {
	GLuint temp;
	glGenVertexArrays(1, &temp);
	DO_ERROR_CHECK();
	return Vao::ptr(new Vao(temp));
}

Vbo::ptr gen_vbo(void) {
	GLuint temp;
	glGenBuffers(1, &temp);
	return Vbo::ptr(new Vbo(temp));
}

Texture::ptr gen_texture(void) {
	GLuint temp;
	glGenTextures(1, &temp);
	DO_ERROR_CHECK();
	return Texture::ptr(new Texture(temp));
}

Shader::ptr gen_shader(GLuint type) {
	GLuint temp = glCreateShader(type);
	DO_ERROR_CHECK();
	return Shader::ptr(new Shader(temp));
}

Program::ptr gen_program(void) {
	GLuint temp = glCreateProgram();
	DO_ERROR_CHECK();
	return Program::ptr(new Program(temp));
}

Framebuffer::ptr gen_framebuffer(void) {
	GLuint temp;
	glGenFramebuffers(1, &temp);
	DO_ERROR_CHECK();
	return Framebuffer::ptr(new Framebuffer(temp));
}

Texture::ptr gen_texture_color(unsigned width, unsigned height, GLenum format) {
	Texture::ptr ret = gen_texture();

	glBindTexture(GL_TEXTURE_2D, ret->obj);
	glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0,
	             GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	return ret;
}

Texture::ptr
gen_texture_depth_stencil(unsigned width, unsigned height, GLenum format) {
	Texture::ptr ret = gen_texture();

	glBindTexture(GL_TEXTURE_2D, ret->obj);
	glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0,
	             GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, nullptr);
	DO_ERROR_CHECK();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	return ret;
}

Vao::ptr get_current_vao(void) {
	return current_vao;
}

Vao::ptr get_screenquad_vao(void) {
	return screenquad_vao;
}

Vao::ptr bind_vao(Vao::ptr v) {
	current_vao = v;
	v->bind();
	DO_ERROR_CHECK();
	return v;
}

GLenum surface_gl_format(SDL_Surface *surf) {
	return surface_gl_format(surf->format->BytesPerPixel);
}

GLenum surface_gl_format(const materialTexture& tex) {
	return surface_gl_format(tex.channels);
}

GLenum surface_gl_format(int channels) {
	switch (channels) {
		case 1: return GL_RED;
		case 2: return GL_RG;
		case 3: return GL_RGB;
		case 4: return GL_RGBA;
		default: break;
	}

	return GL_RGBA;
}

void set_face_order(GLenum face_order) {
	if (face_order != current_face_order) {
		current_face_order = face_order;
		glFrontFace(face_order);
	}
}

void set_default_gl_flags(void) {
	glDepthMask(GL_TRUE);
	enable(GL_DEPTH_TEST);
	// TODO: other flags
}

void enable(GLenum feature) {
	if (feature_cache.find(feature) == feature_cache.end()) {
		glEnable(feature);
		feature_cache.insert(feature);
	}
}

void disable(GLenum feature) {
	auto it = feature_cache.find(feature);

	if (it != feature_cache.end()) {
		glDisable(feature);
		feature_cache.erase(it);
	}
}

// namespace grendx
}
