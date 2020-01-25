#include <grend/grend.hpp>

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

grend::grend() {
	std::cerr << " # Got here, " << __func__ << std::endl;
	std::cerr << " # maximum texture units: " << GL_MAX_TEXTURE_UNITS << std::endl;
	std::cerr << " # maximum combined texture units: " << GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS << std::endl;

	cooked_vertices.clear();
	cooked_normals.clear();
	cooked_elements.clear();
	cooked_texcoords.clear();

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	/*
	// testing texcoord generation
	glEnable(GL_TEXTURE_GEN_S);
	glEnable(GL_TEXTURE_GEN_T);
	glEnable(GL_TEXTURE_GEN_R);
	glEnable(GL_TEXTURE_GEN_Q);
	*/
}

void grend::compile_meshes(std::string objname, const grend::mesh_map& meshies) {
	for (const auto& x : meshies) {
		compiled_mesh foo;
		std::string meshname = objname + "." + x.first;

		//foo.elements_size = x.second.faces.size() * sizeof(GLushort);
		foo.elements_size = x.second.faces.size();
		foo.elements_offset = reinterpret_cast<void*>(cooked_elements.size() * sizeof(GLushort));
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
	fprintf(stderr, " > elements size %lu\n", cooked_elements.size());
}

void grend::compile_models(model_map& models) {
	for (const auto& x : models) {
		std::cerr << " >>> compiling " << x.first << std::endl;
		compiled_model obj;

		obj.vertices_size = x.second.vertices.size() * sizeof(glm::vec3);
		obj.vertices_offset = reinterpret_cast<void*>(cooked_vertices.size() * sizeof(glm::vec3));
		cooked_vertices.insert(cooked_vertices.end(), x.second.vertices.begin(),
		                                              x.second.vertices.end());
		fprintf(stderr, " > vertices size %lu\n", cooked_vertices.size());

		obj.normals_size = x.second.normals.size() * sizeof(glm::vec3);
		obj.normals_offset = reinterpret_cast<void*>(cooked_normals.size() * sizeof(glm::vec3));
		cooked_normals.insert(cooked_normals.end(), x.second.normals.begin(),
		                                            x.second.normals.end());
		fprintf(stderr, " > normals size %lu\n", cooked_normals.size());

		obj.texcoords_size = x.second.texcoords.size() * sizeof(GLfloat) * 2;
		obj.texcoords_offset = reinterpret_cast<void*>(cooked_texcoords.size() * sizeof(GLfloat));
		cooked_texcoords.insert(cooked_texcoords.end(),
				x.second.texcoords.begin(),
				x.second.texcoords.end());
		fprintf(stderr, " > texcoords size %lu\n", cooked_texcoords.size());

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

			if (!mat.second.diffuse_map.empty()) {
				obj.mat_textures[mat.first] = load_texture(mat.second.diffuse_map);
			}
		}

		cooked_models[x.first] = obj;
	}
}

grend::rhandle grend::preload_mesh_vao(compiled_model& obj, compiled_mesh& mesh) {
	rhandle orig_vao = current_vao;
	rhandle ret = bind_vao(gen_vao());

	bind_vbo(cooked_vert_vbo, GL_ARRAY_BUFFER);
	glVertexAttribPointer(cooked_vert_vbo.first, 3, GL_FLOAT, GL_FALSE, 0, obj.vertices_offset);
	enable_vbo(cooked_vert_vbo);

	bind_vbo(cooked_normal_vbo, GL_ARRAY_BUFFER);
	glVertexAttribPointer(cooked_normal_vbo.first, 3, GL_FLOAT, GL_FALSE, 0, obj.normals_offset);
	enable_vbo(cooked_normal_vbo);

	bind_vbo(cooked_texcoord_vbo, GL_ARRAY_BUFFER);
	glVertexAttribPointer(cooked_texcoord_vbo.first, 2, GL_FLOAT, GL_FALSE, 0, obj.texcoords_offset);
	enable_vbo(cooked_texcoord_vbo);

	bind_vbo(cooked_element_vbo, GL_ELEMENT_ARRAY_BUFFER);
	glVertexAttribPointer(cooked_element_vbo.first, 3, GL_UNSIGNED_SHORT, GL_FALSE, 0, mesh.elements_offset);
	enable_vbo(cooked_element_vbo);

	bind_vao(orig_vao);
	return ret;
}

grend::rhandle grend::preload_model_vao(compiled_model& obj) {
	rhandle orig_vao = current_vao;

	for (std::string& mesh_name : obj.meshes) {
		std::cerr << " # binding " << mesh_name << std::endl;
		cooked_meshes[mesh_name].vao = preload_mesh_vao(obj, cooked_meshes[mesh_name]);
	}

	return orig_vao;
}

void grend::bind_cooked_meshes(void) {
	cooked_vert_vbo = gen_vbo();
	cooked_element_vbo = gen_vbo();
	cooked_normal_vbo = gen_vbo();
	cooked_texcoord_vbo = gen_vbo();

	buffer_vbo(cooked_vert_vbo, GL_ARRAY_BUFFER, cooked_vertices);
	buffer_vbo(cooked_normal_vbo, GL_ARRAY_BUFFER, cooked_normals);
	buffer_vbo(cooked_texcoord_vbo, GL_ARRAY_BUFFER, cooked_texcoords);
	buffer_vbo(cooked_element_vbo, GL_ELEMENT_ARRAY_BUFFER, cooked_elements);

	for (auto& x : cooked_models) {
		std::cerr << " # binding " << x.first << std::endl;
		x.second.vao = preload_model_vao(x.second);
	}
}

void grend::free_objects() {
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

grend::rhandle grend::gen_vao(void) {
	GLuint temp;
	glGenVertexArrays(1, &temp);
	vaos.push_back(temp);
	return {temp, vaos.size() - 1};
}

grend::rhandle grend::gen_vbo(void) {
	GLuint temp;
	glGenBuffers(1, &temp);
	vbos.push_back(temp);

	return {temp, vbos.size() - 1};
}

grend::rhandle grend::gen_texture(void) {
	GLuint temp;
	glGenTextures(1, &temp);
	textures.push_back(temp);
	return {temp, textures.size() - 1};
}

grend::rhandle grend::gen_shader(GLuint type) {
	GLuint temp = glCreateShader(type);
	shaders.push_back(temp);
	return {temp, shaders.size() - 1};
}

grend::rhandle grend::gen_program(void) {
	GLuint temp = glCreateProgram();
	programs.push_back(temp);
	return {temp, programs.size() - 1};
}

grend::rhandle grend::bind_vao(const grend::rhandle& handle) {
	current_vao = handle;
	glBindVertexArray(handle.first);
	return handle;
}

grend::rhandle grend::bind_vbo(const grend::rhandle& handle, GLuint type) {
	glBindBuffer(type, handle.first);
	return handle;
}

grend::rhandle grend::va_pointer(const grend::rhandle& handle, GLuint width, GLuint type) {
	glVertexAttribPointer(handle.first, width, type, GL_FALSE, 0, 0);
	return handle;
}

grend::rhandle grend::enable_vbo(const grend::rhandle& handle) {
	glEnableVertexAttribArray(handle.first);
	return handle;
}

grend::rhandle grend::buffer_vbo(const grend::rhandle& handle,
                                 GLuint type,
                                 const std::vector<GLfloat>& vec)
{
	bind_vbo(handle, type);
	glBufferData(type, sizeof(GLfloat) * vec.size(), vec.data(), GL_STATIC_DRAW);
	return handle;
}

grend::rhandle grend::buffer_vbo(const grend::rhandle& handle,
                                 GLuint type,
                                 const std::vector<glm::vec3>& vec)
{
	bind_vbo(handle, type);
	glBufferData(type, sizeof(glm::vec3) * vec.size(), vec.data(), GL_STATIC_DRAW);
	return handle;
}

grend::rhandle grend::buffer_vbo(const grend::rhandle& handle,
                                 GLuint type,
                                 const std::vector<GLushort>& vec)
{
	bind_vbo(handle, type);
	glBufferData(type, sizeof(GLushort) * vec.size(), vec.data(), GL_STATIC_DRAW);
	return handle;
}

grend::rhandle grend::load_vec3f_ab_vattrib(const std::vector<GLfloat>& vec) {
	return va_pointer(enable_vbo(buffer_vbo(gen_vbo(), GL_ARRAY_BUFFER, vec)), 3, GL_FLOAT);
}

grend::rhandle grend::load_vec3f_ab_vattrib(const std::vector<glm::vec3>& vec) {
	return va_pointer(enable_vbo(buffer_vbo(gen_vbo(), GL_ARRAY_BUFFER, vec)), 3, GL_FLOAT);
}

grend::rhandle grend::load_vec3us_eab_vattrib(const std::vector<GLushort>& vec) {
	return va_pointer(enable_vbo(buffer_vbo(gen_vbo(), GL_ELEMENT_ARRAY_BUFFER, vec)), 3, GL_UNSIGNED_SHORT);
}

grend::rhandle grend::load_texture(std::string filename) {
	SDL_Surface *texture = IMG_Load(filename.c_str());

	if (!texture) {
		SDL_Die("Couldn't load texture");
	}

	fprintf(stderr, " > loaded image: w = %u, h = %u, pitch = %u, bytesperpixel: %u\n",
	        texture->w, texture->h, texture->pitch, texture->format->BytesPerPixel);

	GLenum texformat = GL_RGBA;
	switch (texture->format->BytesPerPixel) {
		case 1: texformat = GL_R; break;
		case 2: texformat = GL_RG; break;
		case 3: texformat = GL_RGB; break;
		case 4: texformat = GL_RGBA; break;
		default: break;
	}

	/*
	glGenTextures(1, &texture_id);
	glBindTexture(GL_TEXTURE_2D, texture_id);
	*/
	rhandle temp = gen_texture();
	glBindTexture(GL_TEXTURE_2D, temp.first);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D,
	             0, texformat, texture->w, texture->h, 0, texformat,
	             GL_UNSIGNED_BYTE, texture->pixels);

#if ENABLE_MIPMAPS
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glGenerateMipmap(GL_TEXTURE_2D);
#else
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
#endif

	SDL_FreeSurface(texture);

	return temp;
}

grend::rhandle grend::load_shader(const std::string filename, GLuint type) {
	std::string source = load_file(filename);
	const char *temp = source.c_str();
	int compiled;
	//GLuint ret = glCreateShader(type);
	rhandle ret = gen_shader(type);

	glShaderSource(ret.first, 1, (const GLchar**)&temp, 0);
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

// namespace grendx
}
