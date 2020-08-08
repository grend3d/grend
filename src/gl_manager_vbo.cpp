#include <grend/gl_manager.hpp>
#include <grend/glm-includes.hpp>

#include <string>
#include <vector>
#include <map>

#include <fstream>
#include <iostream>
#include <sstream>

namespace grendx {

void Vbo::buffer(GLuint type,
                 const std::vector<GLfloat>& vec,
                 GLenum usage)
{
	//bind_vbo(handle, type);
	bind(type);
	glBufferData(type, sizeof(GLfloat) * vec.size(), vec.data(), usage);
	DO_ERROR_CHECK();
}

void Vbo::buffer(GLuint type,
                 const std::vector<GLushort>& vec,
                 GLenum usage)
{
	bind(type);
	glBufferData(type, sizeof(GLushort) * vec.size(), vec.data(), usage);
	DO_ERROR_CHECK();
}

void Vbo::buffer(GLuint type,
                 const std::vector<GLuint>& vec,
                 GLenum usage)
{
	bind(type);
	glBufferData(type, sizeof(GLuint) * vec.size(), vec.data(), usage);
	DO_ERROR_CHECK();
}

void Vbo::buffer(GLuint type,
                 const std::vector<glm::vec3>& vec,
                 GLenum usage)
{
	bind(type);
	glBufferData(type, sizeof(glm::vec3) * vec.size(), vec.data(), usage);
	DO_ERROR_CHECK();
}

// namespace grendx
}
