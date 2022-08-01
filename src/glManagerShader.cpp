#include <grend/glManager.hpp>
#include <grend/glmIncludes.hpp>
#include <grend/shaderPreprocess.hpp>
#include <grend/utility.hpp>
#include <grend/glslParser.hpp>
#include <grend/glslObject.hpp>
#include <grend/logger.hpp>

#include <string>
#include <vector>
#include <map>

#include <fstream>
#include <iostream>
#include <sstream>

namespace grendx {

Shader::Shader(GLuint o)
	: Obj(o, Obj::type::Shader) { }

bool Shader::load(std::string filename, const Shader::parameters& options) {
	LogFmt("loading shader: {}", filename);

	std::string source    = load_file(filename);
	std::string processed = preprocessShader(source, options);

	const char *temp = processed.c_str();
	int compiled;

	if (source.empty()) {
		LogFmt("{}: Source file is empty, couldn't load!", filename);
		return false;
	}

	try {
		//auto t = glslObject(source);
		//auto t = parseGlsl(source);
		//dump_tokens_tree(t);

		/*
		if (!t.empty()) {
			LogInfo("Parsed shader successfully");
		} else {
			LogInfo("Couldn't parse shader.");
		}
		*/

	} catch (std::exception& e) {
		LogErrorFmt("Exception, couldn't parse shader: {}", e.what());
	}

	glShaderSource(obj, 1, (const GLchar**)&temp, 0);
	DO_ERROR_CHECK();
	glCompileShader(obj);
	glGetShaderiv(obj, GL_COMPILE_STATUS, &compiled);

	if (!compiled) {
		int max_length;
		char *shader_log;

		glGetShaderiv(obj, GL_INFO_LOG_LENGTH, &max_length);
		shader_log = new char[max_length];
		glGetShaderInfoLog(obj, max_length, &max_length, shader_log);

		LogErrorFmt("BIGERROR: compiliing the processed shader: ");
		LogErrorFmt("@ {}", filename);
		LogErrorFmt("{}", shader_log);
		LogErrorFmt("SOURCE: ----------------------------------");
		LogErrorFmt("{}", processed);

	} else {
		// only store this if the shader is in a good state
		filepath = filename;
		compiledOptions = options;
	}

	return compiled;
}

Shader::parameters mergeOpts(const std::initializer_list<Shader::parameters>& opts) {
	Shader::parameters ret;

	for (auto& optmap : opts) {
		for (auto& [key, value] : optmap) {
			ret[key] = value;
		}
	}

	return ret;
}

// TODO: function that passes new options
bool Shader::reload(void) {
	if (!filepath.empty()) {
		return load(filepath, compiledOptions);
	}

	return false;
}

Program::ptr loadProgram(std::string vert,
                         std::string frag,
                         const Shader::parameters& opts)
{
	Program::ptr prog = genProgram();

	prog->vertex = genShader(GL_VERTEX_SHADER);
	prog->fragment = genShader(GL_FRAGMENT_SHADER);

	prog->vertex->load(vert, opts);
	prog->fragment->load(frag, opts);

	glAttachShader(prog->obj, prog->vertex->obj);
	glAttachShader(prog->obj, prog->fragment->obj);
	DO_ERROR_CHECK();

	return prog;
}

bool Program::link(void) {
	glLinkProgram(obj);
	glGetProgramiv(obj, GL_LINK_STATUS, &linked);

	if (!linked) {
		std::string err = (std::string)"error linking program: " + log();
		LogErrorFmt("{}", err);
	}

	return linked;
}

std::string Program::log(void) {
	int max_length;
	char *prog_log;

	glGetProgramiv(obj, GL_INFO_LOG_LENGTH, &max_length);
	prog_log = new char[max_length];
	glGetProgramInfoLog(obj, max_length, &max_length, prog_log);

	std::string err = std::string(prog_log);
	delete[] prog_log;

	return err;
}

bool Program::reload(void) {
	if (vertex && fragment) {
		if (vertex->reload() && fragment->reload()) {
			for (auto& [attr, location] : attributes) {
				glBindAttribLocation(obj, location, attr.c_str());
			}

			uniforms.clear();
			uniformBlocks.clear();
			storageBlocks.clear();
			objCache.clear();
			valueCache.clear();

			return link();
		}
	}

	return false;
}

void Program::attribute(std::string attr, GLuint location) {
	attributes[attr] = location;
	glBindAttribLocation(obj, location, attr.c_str());
}

#define LOOKUP(U) \
	GLint u = lookup(uniform); \
	if (u < 0) return false;

bool Program::set(std::string uniform, GLint i) {
	LOOKUP(uniform);
	glUniform1i(u, i);
	return true;
}

bool Program::set(std::string uniform, GLfloat f) {
	LOOKUP(uniform);
	glUniform1f(u, f);
	return true;
}

bool Program::set(std::string uniform, glm::vec2 v2) {
	LOOKUP(uniform);
	glUniform2fv(u, 1, glm::value_ptr(v2));
	return true;
}

bool Program::set(std::string uniform, glm::vec3 v3) {
	LOOKUP(uniform);
	glUniform3fv(u, 1, glm::value_ptr(v3));
	return true;
}

bool Program::set(std::string uniform, glm::vec4 v4) {
	LOOKUP(uniform);
	glUniform4fv(u, 1, glm::value_ptr(v4));
	return true;
}

bool Program::set(std::string uniform, glm::mat3 m3) {
	LOOKUP(uniform);
	glUniformMatrix3fv(u, 1, GL_FALSE, glm::value_ptr(m3));
	return true;
}

bool Program::set(std::string uniform, glm::mat4 m4) {
	LOOKUP(uniform);
	glUniformMatrix4fv(u, 1, GL_FALSE, glm::value_ptr(m4));
	return true;
}

// TODO: do the overloads still need to be here if this handles variants...
bool Program::set(std::string uniform, Shader::value val) {
	LOOKUP(uniform);
	DO_ERROR_CHECK();

	if (std::holds_alternative<GLint>(val)) {
		glUniform1i(u, std::get<GLint>(val));
	}

	else if (std::holds_alternative<GLfloat>(val)) {
		glUniform1f(u, std::get<GLfloat>(val));
	}

	else if (std::holds_alternative<glm::vec2>(val)) {
		glUniform2fv(u, 1, glm::value_ptr(std::get<glm::vec2>(val)));
	}

	else if (std::holds_alternative<glm::vec3>(val)) {
		glUniform3fv(u, 1, glm::value_ptr(std::get<glm::vec3>(val)));
	}

	else if (std::holds_alternative<glm::vec4>(val)) {
		glUniform4fv(u, 1, glm::value_ptr(std::get<glm::vec4>(val)));
	}

	else if (std::holds_alternative<glm::mat3>(val)) {
		glUniformMatrix3fv(u, 1, GL_FALSE, glm::value_ptr(std::get<glm::mat3>(val)));
	}

	else if (std::holds_alternative<glm::mat4>(val)) {
		glUniformMatrix4fv(u, 1, GL_FALSE, glm::value_ptr(std::get<glm::mat4>(val)));
	}

	else {
		return false;
	}

	DO_ERROR_CHECK();
	return true;
}

bool Program::setUniformBlock(std::string name, Buffer::ptr buf, GLuint binding) {
	GLuint loc = lookupUniformBlock(name);

	if (loc != GL_INVALID_INDEX) {
		DO_ERROR_CHECK();
		buf->unbind();
		DO_ERROR_CHECK();
		glUniformBlockBinding(obj, loc, binding);
		DO_ERROR_CHECK();
		glBindBufferBase(GL_UNIFORM_BUFFER, binding, buf->obj);
		DO_ERROR_CHECK();
		return true;

	} else {
		return false;
	}
}

bool Program::setStorageBlock(std::string name, Buffer::ptr buf, GLuint binding) {
	// TODO:
	/*
	GLuint loc = lookupStorageBlock(name);
	DO_ERROR_CHECK();
	glUniformBlockBinding(obj, loc, buf->obj);
	*/
	return false;
}

GLint Program::lookup(std::string uniform) {
	auto it = uniforms.find(uniform);

	if (it != uniforms.end()) {
		return it->second;
	}

	return uniforms[uniform] = glGetUniformLocation(obj, uniform.c_str());
}

GLuint Program::lookupUniformBlock(std::string name) {
	auto it = uniformBlocks.find(name);

	if (it != uniformBlocks.end()) {
		return it->second;

	} else {
		GLuint temp = glGetUniformBlockIndex(obj, name.c_str());
		uniformBlocks[name] = temp;
		DO_ERROR_CHECK();

		if (temp != GL_INVALID_INDEX) {
			GLint foo;
			glGetActiveUniformBlockiv(obj, temp, GL_UNIFORM_BLOCK_DATA_SIZE, &foo);

			LogFmt("UBO at {} ({}) has block data size of {}", temp, name, foo);
			return temp;

		} else {
			LogFmt("Could not find uniform block index for {}", name);
			return temp;
		}
	}
}

GLuint Program::lookupStorageBlock(std::string name) {
	auto it = storageBlocks.find(name);

	if (it != storageBlocks.end()) {
		return it->second;

	} else {
		/*
		GLuint temp = glGetUniformBlockIndex(obj, name.c_str());
		uniformBlocks[name] = temp;
		DO_ERROR_CHECK();
		return temp;
		*/
		// TODO:
		return 0;
	}
}

bool Program::cached(std::string uniform) {
	return false;
}

/**
 * Set a cached object entry for the shader.
 *
 * @param name Key for the object cache.
 * @param obj  Object to set as the current object, if not already the current.
 *
 * @return true if a new object was set, false if obj is the current object.
 */
bool Program::cacheObject(const char *name, uintptr_t obj) {
	return true;
	auto it = objCache.find(name);

	if (it == objCache.end() || it->second != obj) {
		objCache[name] = obj;
		return true;

	} else {
		return false;
	}
}

bool Program::cacheObject(const char *name, void *obj) {
	// cram it g++ >:[
	return cacheObject(name, (uintptr_t)obj);
}

// namespace grendx
}
