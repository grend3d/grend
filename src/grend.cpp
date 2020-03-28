#include <grend/gl_manager.hpp>

#include <string>
#include <vector>
#include <map>

#include <fstream>
#include <iostream>
#include <sstream>

// TODO: header for general utility functions (see also split_string())
static std::string load_file(const std::string filename) {
	std::ifstream ifs(filename);
	std::string content((std::istreambuf_iterator<char>(ifs)),
	                    (std::istreambuf_iterator<char>()));

	return content;
}

namespace grendx {

gl_manager::gl_manager() {
	std::cerr << " # Got here, " << __func__ << std::endl;
	std::cerr << " # maximum texture units: " << GL_MAX_TEXTURE_UNITS << std::endl;
	std::cerr << " # maximum combined texture units: " << GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS << std::endl;

	// make sure we start with a bound VAO
	bind_vao(gen_vao());

	// initialize state for quad that fills the screen
	preload_screenquad();

	/*
	cooked_vertices.clear();
	cooked_normals.clear();
	cooked_tangents.clear();
	cooked_bitangents.clear();
	cooked_texcoords.clear();
	*/
	cooked_vertprops.clear();
	cooked_elements.clear();

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_FRAMEBUFFER_SRGB);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDepthFunc(GL_LESS);

	/*
	// testing texcoord generation
	glEnable(GL_TEXTURE_GEN_S);
	glEnable(GL_TEXTURE_GEN_T);
	glEnable(GL_TEXTURE_GEN_R);
	glEnable(GL_TEXTURE_GEN_Q);
	*/
}

void gl_manager::compile_meshes(std::string objname, const mesh_map& meshies) {
	for (const auto& x : meshies) {
		compiled_mesh foo;
		std::string meshname = objname + "." + x.first;

		//foo.elements_size = x.second.faces.size() * sizeof(GLushort);
		foo.elements_size = x.second.faces.size();
		foo.elements_offset = reinterpret_cast<void*>
		                      (cooked_elements.size() * sizeof(GLushort));
		cooked_elements.insert(cooked_elements.end(), x.second.faces.begin(),
		                                              x.second.faces.end());

		// TODO: fix this to work with per-model material maps
		/*
		// check to make sure we have all the materials loaded
		if (materials.find(x.second.material) == materials.end()) {
			std::cerr
				<< "Warning: material \"" << x.second.material
				<< "\" for mesh " << meshname
				<< " isn't loaded!"
				<< std::endl;
			foo.material = "(null)";

		} else {
			foo.material = x.second.material;
		}
		*/

		foo.material = x.second.material;

		cooked_meshes[meshname] = foo;
	}
	//fprintf(stderr, " > elements size %lu\n", cooked_elements.size());
}

// TODO: move
static const size_t VERTPROP_SIZE = (sizeof(glm::vec3[4]) + sizeof(glm::vec2));

void gl_manager::compile_models(model_map& models) {
	for (const auto& x : models) {
		const auto& [mod_name, mod] = x;

		std::cerr << " >>> compiling " << x.first << std::endl;

		assert(mod.vertices.size() == mod.normals.size());
		assert(mod.vertices.size() == mod.texcoords.size());
		assert(mod.vertices.size() == mod.tangents.size());
		assert(mod.vertices.size() == mod.bitangents.size());


		compiled_model obj;
		obj.vertices_size = mod.vertices.size() * VERTPROP_SIZE;

		size_t offset = cooked_vertprops.size() * sizeof(GLfloat);

		auto offset_ptr = [] (uintptr_t off) {
			return reinterpret_cast<void*>(off);
		};

		obj.vertices_offset = offset_ptr(offset);
		obj.normals_offset = offset_ptr(offset + sizeof(glm::vec3[1]));
		obj.tangents_offset = offset_ptr(offset + sizeof(glm::vec3[2]));
		obj.bitangents_offset = offset_ptr(offset + sizeof(glm::vec3[3]));
		obj.texcoords_offset = offset_ptr(offset + sizeof(glm::vec3[4]));

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

		compile_meshes(x.first, x.second.meshes);

		// copy mesh names
		for (const auto& m : x.second.meshes) {
			std::string asdf = x.first + "." + m.first;
			std::cerr << " > have cooked mesh " << asdf << std::endl;
			obj.meshes.push_back(asdf);
		}

		// copy materials
		for (const auto& mat : x.second.materials) {
			obj.materials[mat.first] = mat.second;

			// TODO: is there a less tedious way to do this?
			//       like could load all of the textures into a map then iterate over
			//       the map rather than do this for each map type...
			if (!mat.second.diffuse_map.empty()) {
				obj.mat_textures[mat.first] =
					load_texture(mat.second.diffuse_map, true /* srgb */);
			}

			if (!mat.second.specular_map.empty()) {
				obj.mat_specular[mat.first] = load_texture(mat.second.specular_map);
			}

			if (!mat.second.normal_map.empty()) {
				obj.mat_normal[mat.first] = load_texture(mat.second.normal_map);
			}

			if (!mat.second.ambient_occ_map.empty()) {
				obj.mat_ao[mat.first] = load_texture(mat.second.ambient_occ_map);
			}
		}

		cooked_models[x.first] = obj;
	}
}

gl_manager::rhandle
gl_manager::preload_mesh_vao(compiled_model& obj, compiled_mesh& mesh) {
	rhandle orig_vao = current_vao;
	rhandle ret = bind_vao(gen_vao());

	bind_vbo(cooked_vertprops_vbo, GL_ARRAY_BUFFER);
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

	bind_vbo(cooked_element_vbo, GL_ELEMENT_ARRAY_BUFFER);
	glEnableVertexAttribArray(5);
	glVertexAttribPointer(5, 3, GL_UNSIGNED_SHORT, GL_FALSE, 0,
	                      mesh.elements_offset);

	bind_vao(orig_vao);
	return ret;
}

gl_manager::rhandle gl_manager::preload_model_vao(compiled_model& obj) {
	rhandle orig_vao = current_vao;

	for (std::string& mesh_name : obj.meshes) {
		std::cerr << " # binding " << mesh_name << std::endl;
		cooked_meshes[mesh_name].vao = preload_mesh_vao(obj, cooked_meshes[mesh_name]);
	}

	return orig_vao;
}

void gl_manager::bind_cooked_meshes(void) {
	cooked_vertprops_vbo = gen_vbo();
	cooked_element_vbo = gen_vbo();

	buffer_vbo(cooked_vertprops_vbo, GL_ARRAY_BUFFER, cooked_vertprops);
	buffer_vbo(cooked_element_vbo, GL_ELEMENT_ARRAY_BUFFER, cooked_elements);

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
	rhandle orig_vao = current_vao;
	screenquad_vao = bind_vao(gen_vao());

	buffer_vbo(screenquad_vbo, GL_ARRAY_BUFFER, screenquad_data);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5*sizeof(float), 0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5*sizeof(float),
	                      (void*)(3 * sizeof(float)));

	bind_vao(orig_vao);
}

void gl_manager::free_objects() {
	std::cerr << " # Got here, " << __func__ << std::endl;
	glUseProgram(0);

	fprintf(stderr, "   - %lu shaders\n", shaders.size());
	/*
	for (auto& thing : shaders) {
		// XXX: should detach shaders here, need to keep track of
		//      what shaders are owned where
		// glDetachShader()
	}
	*/

	fprintf(stderr, "   - %lu framebuffers\n", framebuffers.size());
	for (auto& thing : framebuffers) {
		glDeleteFramebuffers(1, &thing);
	}

	fprintf(stderr, "   - %lu programs\n", programs.size());
	for (auto& thing : programs) {
		glDeleteProgram(thing);
	}

	fprintf(stderr, "   - %lu VBOs\n", vbos.size());
	for (auto& thing : vbos) {
		glDeleteBuffers(1, &thing);
	}

	fprintf(stderr, "   - %lu VAOs\n", vaos.size());
	for (auto& thing : vaos) {
		glDeleteVertexArrays(1, &thing);
	}

	fprintf(stderr, "   - %lu textures\n", textures.size());
	for (auto& thing : textures) {
		glDeleteTextures(1, &thing);
	}

	std::cerr << " # done cleanup" << std::endl;
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

gl_manager::rhandle gl_manager::gen_vao(void) {
	GLuint temp;
	glGenVertexArrays(1, &temp);
	DO_ERROR_CHECK();
	vaos.push_back(temp);
	return {temp, vaos.size() - 1};
}

gl_manager::rhandle gl_manager::gen_vbo(void) {
	GLuint temp;
	glGenBuffers(1, &temp);
	DO_ERROR_CHECK();
	vbos.push_back(temp);

	return {temp, vbos.size() - 1};
}

gl_manager::rhandle gl_manager::gen_texture(void) {
	GLuint temp;
	glGenTextures(1, &temp);
	DO_ERROR_CHECK();
	textures.push_back(temp);
	return {temp, textures.size() - 1};
}

gl_manager::rhandle gl_manager::gen_shader(GLuint type) {
	GLuint temp = glCreateShader(type);
	DO_ERROR_CHECK();
	shaders.push_back(temp);
	return {temp, shaders.size() - 1};
}

gl_manager::rhandle gl_manager::gen_program(void) {
	GLuint temp = glCreateProgram();
	DO_ERROR_CHECK();
	programs.push_back(temp);
	return {temp, programs.size() - 1};
}

gl_manager::rhandle gl_manager::gen_framebuffer(void) {
	GLuint temp;
	glGenFramebuffers(1, &temp);
	DO_ERROR_CHECK();
	framebuffers.push_back(temp);
	return {temp, framebuffers.size() - 1};
}

gl_manager::rhandle
gl_manager::gen_texture_color(unsigned width, unsigned height, GLenum format) {
	rhandle ret = gen_texture();

	glBindTexture(GL_TEXTURE_2D, ret.first);
	glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0,
	             GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	return ret;
}

gl_manager::rhandle
gl_manager::gen_texture_depth_stencil(unsigned width, unsigned height,
                                      GLenum format)
{
	rhandle ret = gen_texture();

	glBindTexture(GL_TEXTURE_2D, ret.first);
	glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0,
	             GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, nullptr);
	DO_ERROR_CHECK();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	return ret;
}

gl_manager::rhandle gl_manager::bind_vao(const gl_manager::rhandle& handle) {
	current_vao = handle;
	glBindVertexArray(handle.first);
	DO_ERROR_CHECK();
	return handle;
}

gl_manager::rhandle
gl_manager::bind_vbo(const gl_manager::rhandle& handle, GLuint type) {
	glBindBuffer(type, handle.first);
	DO_ERROR_CHECK();
	return handle;
}

gl_manager::rhandle
gl_manager::bind_framebuffer(const rhandle& handle) {
	glBindFramebuffer(GL_FRAMEBUFFER, handle.first);
	DO_ERROR_CHECK();
	return handle;
}

void gl_manager::bind_default_framebuffer(void) {
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	DO_ERROR_CHECK();
}

gl_manager::rhandle
gl_manager::va_pointer(const gl_manager::rhandle& handle,
                       GLuint width,
                       GLuint type)
{
	glVertexAttribPointer(handle.first, width, type, GL_FALSE, 0, 0);
	DO_ERROR_CHECK();
	return handle;
}

gl_manager::rhandle gl_manager::buffer_vbo(const gl_manager::rhandle& handle,
                                 GLuint type,
                                 const std::vector<GLfloat>& vec,
                                 GLenum usage)
{
	bind_vbo(handle, type);
	glBufferData(type, sizeof(GLfloat) * vec.size(), vec.data(), usage);
	DO_ERROR_CHECK();
	return handle;
}

gl_manager::rhandle
gl_manager::buffer_vbo(const gl_manager::rhandle& handle,
                       GLuint type,
                       const std::vector<glm::vec3>& vec,
                       GLenum usage)
{
	bind_vbo(handle, type);
	glBufferData(type, sizeof(glm::vec3) * vec.size(), vec.data(), usage);
	DO_ERROR_CHECK();
	return handle;
}

gl_manager::rhandle
gl_manager::buffer_vbo(const gl_manager::rhandle& handle,
                       GLuint type,
                       const std::vector<GLushort>& vec,
                       GLenum usage)
{
	bind_vbo(handle, type);
	glBufferData(type, sizeof(GLushort) * vec.size(), vec.data(), usage);
	DO_ERROR_CHECK();
	return handle;
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

gl_manager::rhandle gl_manager::load_texture(std::string filename, bool srgb) {
	if (texture_cache.find(filename) != texture_cache.end()) {
		// avoid redundantly loading textures
		//std::cerr << " > cached texture " << filename << std::endl;
		return texture_cache[filename];
	}
	//std::cerr << " > loading texture " << filename << std::endl;

	SDL_Surface *texture = IMG_Load(filename.c_str());

	if (!texture) {
		SDL_Die("Couldn't load texture");
	}

	fprintf(stderr, " > loaded image: w = %u, h = %u, pitch = %u, bytesperpixel: %u\n",
	        texture->w, texture->h, texture->pitch, texture->format->BytesPerPixel);

	GLenum texformat = surface_gl_format(texture);
	/*
	glGenTextures(1, &texture_id);
	glBindTexture(GL_TEXTURE_2D, texture_id);
	*/
	rhandle temp = gen_texture();
	glBindTexture(GL_TEXTURE_2D, temp.first);
	DO_ERROR_CHECK();
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D,
	             //0, GL_RGBA, texture->w, texture->h, 0, texformat,
	             0, srgb? GL_SRGB_ALPHA : GL_RGBA, texture->w, texture->h,
	             0, texformat, GL_UNSIGNED_BYTE, texture->pixels);
	DO_ERROR_CHECK();

#if ENABLE_MIPMAPS
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	DO_ERROR_CHECK();
	glGenerateMipmap(GL_TEXTURE_2D);
#else
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
#endif
	DO_ERROR_CHECK();

	SDL_FreeSurface(texture);

	texture_cache[filename] = temp;
	return temp;
}

gl_manager::rhandle
gl_manager::load_cubemap(std::string directory, std::string extension) {
	// TODO: texture cache
	rhandle tex = gen_texture();

	glBindTexture(GL_TEXTURE_CUBE_MAP, tex.first);
	DO_ERROR_CHECK();

	std::pair<std::string, GLenum> dirmap[] =  {
		{"negx", GL_TEXTURE_CUBE_MAP_NEGATIVE_X},
		{"negy", GL_TEXTURE_CUBE_MAP_NEGATIVE_Y},
		{"negz", GL_TEXTURE_CUBE_MAP_NEGATIVE_Z},
		{"posx", GL_TEXTURE_CUBE_MAP_POSITIVE_X},
		{"posy", GL_TEXTURE_CUBE_MAP_POSITIVE_Y},
		{"posz", GL_TEXTURE_CUBE_MAP_POSITIVE_Z}
	};

	for (const auto& thing : dirmap) {
		std::string fname = directory + "/" + thing.first + extension;
		SDL_Surface *surf = IMG_Load(fname.c_str());

		if (!surf) {
			SDL_Die("couldn't load cubemap texture");
		}

		fprintf(stderr, " > loaded image: w = %u, h = %u, pitch = %u, bytesperpixel: %u\n",
	        surf->w, surf->h, surf->pitch, surf->format->BytesPerPixel);

		/*
		glTexImage2D(thing.second, 0, GL_RGBA, surf->w, surf->h, 0,
			surface_gl_format(surf), GL_UNSIGNED_BYTE, surf->pixels);
			*/
		glTexImage2D(thing.second, 0, GL_SRGB, surf->w, surf->h, 0,
			surface_gl_format(surf), GL_UNSIGNED_BYTE, surf->pixels);
		DO_ERROR_CHECK();

		SDL_FreeSurface(surf);
	}

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	DO_ERROR_CHECK();

	return tex;
}

gl_manager::rhandle
gl_manager::load_shader(const std::string filename, GLuint type) {
	std::string source = load_file(filename);
	const char *temp = source.c_str();
	int compiled;
	//GLuint ret = glCreateShader(type);
	rhandle ret = gen_shader(type);

	glShaderSource(ret.first, 1, (const GLchar**)&temp, 0);
	DO_ERROR_CHECK();
	glCompileShader(ret.first);
	glGetShaderiv(ret.first, GL_COMPILE_STATUS, &compiled);

	if (!compiled) {
		int max_length;
		char *shader_log;

		glGetShaderiv(ret.first, GL_INFO_LOG_LENGTH, &max_length);
		shader_log = new char[max_length];
		glGetShaderInfoLog(ret.first, max_length, &max_length, shader_log);

		throw std::logic_error(filename + ": " + shader_log);
	}

	return ret;
}

gl_manager::rhandle
gl_manager::fb_attach_texture(GLenum attachment, const rhandle& texture)
{
	glBindTexture(GL_TEXTURE_2D, texture.first);
	glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D,
	                       texture.first, 0);
	DO_ERROR_CHECK();

	return texture;
}

// namespace grendx
}
