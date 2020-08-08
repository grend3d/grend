#include <grend/gl_manager.hpp>

#include <tinygltf/stb_image.h>
#include <tinygltf/stb_image_write.h>

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
	for (const auto& mat : mod.materials) {
		//obj.materials[mat.first] = mat.second;
		obj.materials[mat.first].copy_properties(mat.second);

		// TODO: is there a less tedious way to do this?
		//       like could load all of the textures into a map then iterate over
		//       the map rather than do this for each map type...
		if (mat.second.diffuse_map.loaded()) {
			obj.mat_textures[mat.first] =
				buffer_texture(mat.second.diffuse_map, true /* srgb */);
		}

		if (mat.second.metal_roughness_map.loaded()) {
			obj.mat_specular[mat.first] =
				buffer_texture(mat.second.metal_roughness_map);
		}

		if (mat.second.normal_map.loaded()) {
			obj.mat_normal[mat.first] =
				buffer_texture(mat.second.normal_map);
		}

		if (mat.second.ambient_occ_map.loaded()) {
			obj.mat_ao[mat.first] =
				buffer_texture(mat.second.ambient_occ_map);
		}
	}

	cooked_models[name] = obj;
}

void gl_manager::compile_models(model_map& models) {
	for (const auto& x : models) {
		compile_model(x.first, x.second);
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
	glVertexAttribPointer(5, 3, GL_UNSIGNED_INT, GL_FALSE, 0,
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

	rhandle ret = {temp, programs.size() - 1};
	shader_objs.push_back(shader(ret));

	return ret;
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
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

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
	if (handle == current_fb) {
		return current_fb;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, handle.first);
	current_fb = handle;
	DO_ERROR_CHECK();
	return handle;
}

void gl_manager::bind_default_framebuffer(void) {
	if (current_fb == rhandle(0, 0)) {
		return;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	current_fb = {0, 0};
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

gl_manager::rhandle
gl_manager::buffer_vbo(const gl_manager::rhandle& handle,
                       GLuint type,
                       const std::vector<GLuint>& vec,
                       GLenum usage)
{
	bind_vbo(handle, type);
	glBufferData(type, sizeof(GLuint) * vec.size(), vec.data(), usage);
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

uint32_t dumbhash(const std::vector<uint8_t>& pixels) {
	uint32_t ret = 757;

	for (uint8_t c : pixels) {
		ret = ((ret << 7) + ret + c);
	}

	return ret;
}

gl_manager::rhandle
gl_manager::buffer_texture(const material_texture& tex, bool srgb) {
	uint32_t texhash = dumbhash(tex.pixels);

	if (texture_cache.find(texhash) != texture_cache.end()) {
		// avoid redundantly loading textures
		//std::cerr << " > cached texture " << filename << std::endl;
		//return texture_cache[filename];

		std::cerr << " > cached texture " << std::hex << texhash
			<< std::dec << std::endl;
		return texture_cache[texhash];
	}

	/*
	//std::cerr << " > loading texture " << filename << std::endl;

	SDL_Surface *texture = IMG_Load(filename.c_str());

	if (!texture) {
		SDL_Die("Couldn't load texture");
	}
	*/

	fprintf(stderr, " > buffering image: w = %u, h = %u, bytesperpixel: %u\n",
	        tex.width, tex.height, tex.channels);

	GLenum texformat = surface_gl_format(tex);
	/*
	glGenTextures(1, &texture_id);
	glBindTexture(GL_TEXTURE_2D, texture_id);
	*/
	rhandle temp = gen_texture();
	glBindTexture(GL_TEXTURE_2D, temp.first);
	DO_ERROR_CHECK();

#ifdef NO_FORMAT_CONVERSION
	// TODO: fallback SRBG conversion
	glTexImage2D(GL_TEXTURE_2D,
	             //0, srgb? GL_SRGB_ALPHA : GL_RGBA, tex.width, tex.height,
	             0, texformat, tex.width, tex.height,
	             0, texformat, GL_UNSIGNED_BYTE, tex.pixels.data());

#else
	glTexImage2D(GL_TEXTURE_2D,
	             0, srgb? GL_SRGB_ALPHA : GL_RGBA, tex.width, tex.height,
	             0, texformat, GL_UNSIGNED_BYTE, tex.pixels.data());
#endif

	DO_ERROR_CHECK();

#if ENABLE_MIPMAPS
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	DO_ERROR_CHECK();
	glGenerateMipmap(GL_TEXTURE_2D);
#else
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
#endif
	DO_ERROR_CHECK();

	/*
	SDL_FreeSurface(texture);

	texture_cache[filename] = temp;
	*/
	texture_cache[texhash] = temp;
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

	std::cerr << __func__ << ": hi" << std::endl;

	for (const auto& thing : dirmap) {
		std::string fname = directory + "/" + thing.first + extension;
		//SDL_Surface *surf = IMG_Load(fname.c_str());
		int width, height, channels;
		uint8_t *surf = stbi_load(fname.c_str(), &width, &height, &channels, 0);
		std::cerr << __func__ << ": loaded a " << fname << std::endl;

		if (!surf) {
			SDL_Die("couldn't load cubemap texture");
		}

		fprintf(stderr, " > loaded image: w = %u, h = %u, pitch = %u, bytesperpixel: %u\n",
	        width, height, 0, channels);

		GLenum texformat = surface_gl_format(channels);

#ifdef NO_FORMAT_CONVERSION
		// TODO: fallback srgb conversion
		glTexImage2D(thing.second, 0, texformat, width, height, 0,
			texformat, GL_UNSIGNED_BYTE, surf);
#else
		glTexImage2D(thing.second, 0, GL_SRGB, width, height, 0,
			texformat, GL_UNSIGNED_BYTE, surf);
#endif
		DO_ERROR_CHECK();

		//SDL_FreeSurface(surf);
		stbi_image_free(surf);

		for (unsigned i = 1; surf; i++) {
			std::string mipname = directory + "/mip" + std::to_string(i)
			                      + "-" + thing.first + extension;
			std::cerr << " > looking for mipmap " << mipname << std::endl;

			// TODO: change this to stb image
			//surf = IMG_Load(mipname.c_str());
			surf = stbi_load(mipname.c_str(), &width, &height, &channels, 0);
			if (!surf) break;

			// maybe this?
			/*
			if (!surf){
				glTexParameteri(thing.second, GL_TEXTURE_BASE_LEVEL, 0);
				glTexParameteri(thing.second, GL_TEXTURE_MAX_LEVEL, i - 1);
				DO_ERROR_CHECK();
				break;
			}
			*/

			fprintf(stderr, " > loaded image: w = %u, h = %u, pitch = %u, bytesperpixel: %u\n",
				width, height, 0, channels);


			GLenum texformat = surface_gl_format(channels);
#ifdef NO_FORMAT_CONVERSION
			// again, SRGB fallback, also maybe have a function to init textures
			glTexImage2D(thing.second, i, texformat, width, height, 0,
				texformat, GL_UNSIGNED_BYTE, surf);
#else
			glTexImage2D(thing.second, i, GL_SRGB, width, height, 0,
				texformat, GL_UNSIGNED_BYTE, surf);
#endif

			DO_ERROR_CHECK();

			stbi_image_free(surf);
		}
	}

	//glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	DO_ERROR_CHECK();

	//glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
#if GLSL_VERSION >= 130
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
#endif
	DO_ERROR_CHECK();

	return tex;
}

gl_manager::rhandle
gl_manager::load_shader(const std::string filename, GLuint type) {
	std::cerr << __func__ << ": " << __LINE__ << ": loading "
	          << filename << std::endl;

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

		std::string err = (filename + ": " + shader_log);
		SDL_Die(err.c_str());
		//throw std::logic_error(filename + ": " + shader_log);
	}

	return ret;
}

gl_manager::rhandle gl_manager::load_program(std::string vert, std::string frag) {
	rhandle vertsh, fragsh, prog;

	vertsh = load_shader(vert, GL_VERTEX_SHADER);
	fragsh = load_shader(frag, GL_FRAGMENT_SHADER);
	prog = gen_program();

	glAttachShader(prog.first, vertsh.first);
	glAttachShader(prog.first, fragsh.first);
	DO_ERROR_CHECK();

	return prog;
}

gl_manager::rhandle gl_manager::link_program(rhandle program) {
	int linked;

	glLinkProgram(program.first);
	glGetProgramiv(program.first, GL_LINK_STATUS, &linked);

	if (!linked) {
		int max_length;
		char *prog_log;

		glGetProgramiv(program.first, GL_INFO_LOG_LENGTH, &max_length);
		prog_log = new char[max_length];
		glGetProgramInfoLog(program.first, max_length, &max_length, prog_log);

		std::string err = (std::string)"error linking program: " + prog_log;
		delete prog_log;

		SDL_Die(err.c_str());
	}

	return program;
}

gl_manager::rhandle
gl_manager::fb_attach_texture(GLenum attachment, const rhandle& texture)
{
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture.first);
	glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D,
	                       texture.first, 0);
	DO_ERROR_CHECK();

	return texture;
}

const gl_manager::shader& gl_manager::get_shader_obj(rhandle& handle) {
	assert(handle.second >= 0
	       && handle.second < programs.size()
	       && handle.second < shader_objs.size());

	return shader_objs[handle.second];
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