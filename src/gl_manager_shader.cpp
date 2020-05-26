#include <grend/gl_manager.hpp>
#include <grend/glm-includes.hpp>

#include <string>
#include <vector>
#include <map>

#include <fstream>
#include <iostream>
#include <sstream>

namespace grendx {

#define LOOKUP(U) \
	GLint u = lookup(uniform); \
	if (u < 0) return;

void gl_manager::shader::set(std::string uniform, GLint i) {
	LOOKUP(uniform);
	glUniform1i(u, i);
}

void gl_manager::shader::set(std::string uniform, GLfloat f) {
	LOOKUP(uniform);
	glUniform1f(u, f);
}

void gl_manager::shader::set(std::string uniform, glm::vec2 v2) {
	LOOKUP(uniform);
	glUniform2fv(u, 1, glm::value_ptr(v2));
}

void gl_manager::shader::set(std::string uniform, glm::vec3 v3) {
	LOOKUP(uniform);
	glUniform3fv(u, 1, glm::value_ptr(v3));
}

void gl_manager::shader::set(std::string uniform, glm::vec4 v4) {
	LOOKUP(uniform);
	glUniform4fv(u, 1, glm::value_ptr(v4));
}

void gl_manager::shader::set(std::string uniform, glm::mat3 m3) {
	LOOKUP(uniform);
	glUniformMatrix3fv(u, 1, GL_FALSE, glm::value_ptr(m3));
}

void gl_manager::shader::set(std::string uniform, glm::mat4 m4) {
	LOOKUP(uniform);
	glUniformMatrix4fv(u, 1, GL_FALSE, glm::value_ptr(m4));
}

GLint gl_manager::shader::lookup(std::string uniform) {
	auto it = uniforms.find(uniform);

	if (it != uniforms.end()) {
		return it->second;
	}

	return uniforms[uniform] = glGetUniformLocation(handle.first, uniform.c_str());
}

bool gl_manager::shader::cached(std::string uniform) {
	return false;
}

// namespace grendx
}
