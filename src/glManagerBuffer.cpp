#include <grend/glManager.hpp>
#include <grend/glmIncludes.hpp>

#include <string>
#include <vector>
#include <map>

#include <fstream>
#include <iostream>
#include <sstream>

namespace grendx {

void Buffer::bind(void) {
	glBindBuffer(type, obj);
	DO_ERROR_CHECK();
}

void Buffer::unbind(void) {
	glBindBuffer(type, 0);
	DO_ERROR_CHECK();
}

void *Buffer::map(GLenum access) {
	//return glMapBuffer(type, access);
	DO_ERROR_CHECK();
}

GLenum Buffer::unmap(void) {
	return glUnmapBuffer(type);
	DO_ERROR_CHECK();
}

void Buffer::allocate(size_t n) {
	bind();
	glBufferData(type, n, NULL, use);
	DO_ERROR_CHECK();
}

void Buffer::update(const void *ptr, size_t offset, size_t n) {
	bind();
	glBufferSubData(type, offset, n, ptr);
	DO_ERROR_CHECK();
}

void Buffer::buffer(const void *ptr, size_t n) {
	bind();
	glBufferData(type, n, ptr, use);
	DO_ERROR_CHECK();

	currentSize = glmanDbgUpdateBuffered(currentSize, n);
}

void Buffer::buffer(const std::vector<GLfloat>& vec) {
	bind();
	glBufferData(type, sizeof(GLfloat) * vec.size(), vec.data(), use);
	DO_ERROR_CHECK();

	size_t vecbytes = sizeof(GLfloat) * vec.size();
	currentSize = glmanDbgUpdateBuffered(currentSize, vecbytes);
}

void Buffer::buffer(const std::vector<GLushort>& vec) {
	bind();
	glBufferData(type, sizeof(GLushort) * vec.size(), vec.data(), use);
	DO_ERROR_CHECK();

	size_t vecbytes = sizeof(GLushort) * vec.size();
	currentSize = glmanDbgUpdateBuffered(currentSize, vecbytes);
}

void Buffer::buffer(const std::vector<GLuint>& vec) {
	bind();
	glBufferData(type, sizeof(GLuint) * vec.size(), vec.data(), use);
	DO_ERROR_CHECK();

	size_t vecbytes = sizeof(GLuint) * vec.size();
	currentSize = glmanDbgUpdateBuffered(currentSize, vecbytes);
}

void Buffer::buffer(const std::vector<glm::vec3>& vec) {
	bind();
	glBufferData(type, sizeof(glm::vec3) * vec.size(), vec.data(), use);
	DO_ERROR_CHECK();

	size_t vecbytes = sizeof(glm::vec3) * vec.size();
	currentSize = glmanDbgUpdateBuffered(currentSize, vecbytes);
}

// namespace grendx
}
