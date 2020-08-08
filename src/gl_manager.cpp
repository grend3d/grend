#include <grend/gl_manager.hpp>

#include <string>
#include <vector>
#include <map>

#include <fstream>
#include <iostream>
#include <sstream>

namespace grendx {

gl_manager::gl_manager() {
	std::cerr << " # Got here, " << __func__ << std::endl;
	std::cerr << " # maximum texture units: " << GL_MAX_TEXTURE_UNITS << std::endl;
	std::cerr << " # maximum combined texture units: " << GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS << std::endl;

	// make sure we start with a bound VAO
	bind_vao(gen_vao());

	// initialize state for quad that fills the screen
	preload_screenquad();

	cooked_vertprops.clear();
	cooked_elements.clear();

	cooked_vertprops_vbo = gen_vbo();
	cooked_element_vbo = gen_vbo();
}

void gl_manager::compile_meshes(std::string objname, const mesh_map& meshies) {
	for (const auto& x : meshies) {
		compiled_mesh foo;
		std::string meshname = objname + "." + x.first;

		//foo.elements_size = x.second.faces.size() * sizeof(GLushort);
		foo.elements_size = x.second.faces.size();
		foo.elements_offset = reinterpret_cast<void*>
		                      (cooked_elements.size() * sizeof(GLuint));
		cooked_elements.insert(cooked_elements.end(), x.second.faces.begin(),
		                                              x.second.faces.end());
		foo.material = x.second.material;

		cooked_meshes[meshname] = foo;
	}
	//fprintf(stderr, " > elements size %lu\n", cooked_elements.size());
}

// TODO: move
static const size_t VERTPROP_SIZE = (sizeof(glm::vec3[4]) + sizeof(glm::vec2));

void gl_manager::compile_model(std::string name, const model& mod) {
	std::cerr << " >>> compiling " << name << std::endl;

	assert(mod.vertices.size() == mod.normals.size());
	assert(mod.vertices.size() == mod.texcoords.size());
	//assert(mod.vertices.size() == mod.tangents.size());
	//assert(mod.vertices.size() == mod.bitangents.size());

	compiled_model obj;
	obj.vertices_size = mod.vertices.size() * VERTPROP_SIZE;

	size_t offset = cooked_vertprops.size() * sizeof(GLfloat);

	auto offset_ptr = [] (uintptr_t off) {
		return reinterpret_cast<void*>(off);
	};

	obj.vertices_offset   = offset_ptr(offset);
	obj.normals_offset    = offset_ptr(offset + sizeof(glm::vec3[1]));
	obj.tangents_offset   = offset_ptr(offset + sizeof(glm::vec3[2]));
	obj.bitangents_offset = offset_ptr(offset + sizeof(glm::vec3[3]));
	obj.texcoords_offset  = offset_ptr(offset + sizeof(glm::vec3[4]));

	for (unsigned i = 0; i < mod.vertices.size(); i++) {
		// XXX: glm vectors don't have an iterator
		auto push_vec = [&] (const auto& vec) {
			for (unsigned k = 0; k < vec.length(); k++) {
				cooked_vertprops.push_back(vec[k]);
			}
		};

		push_vec(mod.vertices[i]);
		push_vec(mod.normals[i]);
		push_vec(mod.tangents[i]);
		push_vec(mod.bitangents[i]);
		push_vec(mod.texcoords[i]);
	}

	compile_meshes(name, mod.meshes);

	// copy mesh names
	for (const auto& m : mod.meshes) {
		std::string asdf = name + "." + m.first;
		std::cerr << " > have cooked mesh " << asdf << std::endl;
		obj.meshes.push_back(asdf);
	}

	// copy materials
	for (const auto& [name, mat] : mod.materials) {
		//obj.materials[name] = mat;
		obj.materials[name].copy_properties(mat);

		// TODO: is there a less tedious way to do this?
		//       like could load all of the textures into a map then iterate over
		//       the map rather than do this for each map type...
		if (mat.diffuse_map.loaded()) {
			obj.mat_textures[name] = gen_texture();
			obj.mat_textures[name]->buffer(mat.diffuse_map, true /* srgb */);
		}

		if (mat.metal_roughness_map.loaded()) {
			obj.mat_specular[name] = gen_texture();
			obj.mat_specular[name]->buffer(mat.metal_roughness_map);
		}

		if (mat.normal_map.loaded()) {
			obj.mat_normal[name] = gen_texture();
			obj.mat_normal[name]->buffer(mat.normal_map);
		}

		if (mat.ambient_occ_map.loaded()) {
			obj.mat_ao[name] = gen_texture();
			obj.mat_ao[name]->buffer(mat.ambient_occ_map);
		}
	}

	cooked_models[name] = obj;
}

void gl_manager::compile_models(model_map& models) {
	for (const auto& x : models) {
		compile_model(x.first, x.second);
	}
}

Vao::ptr
gl_manager::preload_mesh_vao(compiled_model& obj, compiled_mesh& mesh) {
	Vao::ptr orig_vao = current_vao;
	Vao::ptr ret = bind_vao(gen_vao());

	cooked_vertprops_vbo->bind(GL_ARRAY_BUFFER);

	//bind_vbo(cooked_vertprops_vbo, GL_ARRAY_BUFFER);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, VERTPROP_SIZE,
	                      obj.vertices_offset);

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, VERTPROP_SIZE,
	                      obj.normals_offset);

	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, VERTPROP_SIZE,
	                      obj.tangents_offset);

	glEnableVertexAttribArray(3);
	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, VERTPROP_SIZE,
	                      obj.bitangents_offset);

	glEnableVertexAttribArray(4);
	glVertexAttribPointer(4, 2, GL_FLOAT, GL_FALSE, VERTPROP_SIZE,
	                      obj.texcoords_offset);

	//bind_vbo(cooked_element_vbo, GL_ELEMENT_ARRAY_BUFFER);
	cooked_element_vbo->bind(GL_ELEMENT_ARRAY_BUFFER);
	glEnableVertexAttribArray(5);
	glVertexAttribPointer(5, 3, GL_UNSIGNED_INT, GL_FALSE, 0,
	                      mesh.elements_offset);

	bind_vao(orig_vao);
	return ret;
}

Vao::ptr gl_manager::preload_model_vao(compiled_model& obj) {
	Vao::ptr orig_vao = current_vao;

	for (std::string& mesh_name : obj.meshes) {
		std::cerr << " # binding " << mesh_name << std::endl;
		cooked_meshes[mesh_name].vao = preload_mesh_vao(obj, cooked_meshes[mesh_name]);
	}

	return orig_vao;
}

void gl_manager::bind_cooked_meshes(void) {
	cooked_vertprops_vbo->buffer(GL_ARRAY_BUFFER, cooked_vertprops);
	cooked_element_vbo->buffer(GL_ELEMENT_ARRAY_BUFFER, cooked_elements);

	for (auto& x : cooked_models) {
		std::cerr << " # binding " << x.first << std::endl;
		x.second.vao = preload_model_vao(x.second);
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

void gl_manager::preload_screenquad(void) {
	screenquad_vbo = gen_vbo();
	Vao::ptr orig_vao = current_vao;
	screenquad_vao = bind_vao(gen_vao());

	screenquad_vbo->buffer(GL_ARRAY_BUFFER, screenquad_data);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5*sizeof(float), 0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5*sizeof(float),
	                      (void*)(3 * sizeof(float)));

	bind_vao(orig_vao);
}

void check_errors(int line, const char *func) {
	GLenum err;

	// TODO: maybe exceptions or some way to address errors
	while ((err = glGetError()) != GL_NO_ERROR) {
		std::cerr << "/!\\ ERROR: " << func << ":" << line << ", ";

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

Vao::ptr gl_manager::bind_vao(Vao::ptr v) {
	current_vao = v;
	v->bind();
	DO_ERROR_CHECK();
	return v;
}

GLenum surface_gl_format(SDL_Surface *surf) {
	switch (surf->format->BytesPerPixel) {
		case 1: return GL_RED;
		case 2: return GL_RG;
		case 3: return GL_RGB;
		case 4: return GL_RGBA;
		default: break;
	}

	return GL_RGBA;
}

GLenum surface_gl_format(const material_texture& tex) {
	switch (tex.channels) {
		case 1: return GL_RED;
		case 2: return GL_RG;
		case 3: return GL_RGB;
		case 4: return GL_RGBA;
		default: break;
	}

	return GL_RGBA;
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

Program::ptr load_program(std::string vert, std::string frag) {
	Program::ptr prog = gen_program();

	prog->vertex = gen_shader(GL_VERTEX_SHADER);
	prog->fragment = gen_shader(GL_FRAGMENT_SHADER);

	prog->vertex->load(vert);
	prog->fragment->load(frag);

	glAttachShader(prog->obj, prog->vertex->obj);
	glAttachShader(prog->obj, prog->fragment->obj);
	DO_ERROR_CHECK();

	return prog;
}

Program::ptr link_program(Program::ptr program) {
	int linked;

	glLinkProgram(program->obj);
	glGetProgramiv(program->obj, GL_LINK_STATUS, &linked);

	if (!linked) {
		int max_length;
		char *prog_log;

		glGetProgramiv(program->obj, GL_INFO_LOG_LENGTH, &max_length);
		prog_log = new char[max_length];
		glGetProgramInfoLog(program->obj, max_length, &max_length, prog_log);

		std::string err = (std::string)"error linking program: " + prog_log;
		delete prog_log;

		std::cerr << err << std::endl;
	}

	return program;
}

void gl_manager::set_face_order(GLenum face_order) {
	if (face_order != current_face_order) {
		current_face_order = face_order;
		glFrontFace(face_order);
	}
}

void gl_manager::enable(GLenum feature) {
	if (feature_cache.find(feature) == feature_cache.end()) {
		glEnable(feature);
		feature_cache.insert(feature);
	}
}

void gl_manager::disable(GLenum feature) {
	auto it = feature_cache.find(feature);

	if (it != feature_cache.end()) {
		glDisable(feature);
		feature_cache.erase(it);
	}
}

// namespace grendx
}
