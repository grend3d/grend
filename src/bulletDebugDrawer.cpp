#include <grend-config.h>

#ifdef PHYSICS_BULLET
#include <grend/physics.hpp>
#include <grend/bulletPhysics.hpp>
#include <grend/glManager.hpp>
//#include "btBulletDynamicsCommon.h"

using namespace grendx;

bulletDebugDrawer::~bulletDebugDrawer() {};

// XXX: maybe these macros should be in glManager.hpp, they're very convenient
//      (duplicated from glManager.cpp)
#define STRUCT_OFFSET(TYPE, MEMBER) \
	((void *)&(((TYPE*)NULL)->MEMBER))

// constexpr, should be fine right?
// otherwise can just assume 4 bytes, do sizeof()
#define GLM_VEC_ENTRIES(TYPE, MEMBER) \
	((((TYPE*)NULL)->MEMBER).length())

#define SET_VAO_ENTRY(BINDING, TYPE, MEMBER) \
	do { \
	glVertexAttribPointer(BINDING, GLM_VEC_ENTRIES(TYPE, MEMBER), \
	                      GL_FLOAT, GL_FALSE, sizeof(TYPE),       \
	                      STRUCT_OFFSET(TYPE, MEMBER));           \
	} while(0);

bulletDebugDrawer::bulletDebugDrawer() {
	databuf = genBuffer(GL_ARRAY_BUFFER, GL_STREAM_DRAW);
	vao = bindVao(genVao());

	Shader::parameters noopts; // XXX: this is dumb
	shader = loadProgram(GR_PREFIX "shaders/baked/linedraw.vert",
	                     GR_PREFIX "shaders/baked/linedraw.frag", noopts);
	shader->attribute("v_position", 0);
	shader->attribute("v_color", 1);
	shader->link();

	databuf->bind();
	glEnableVertexAttribArray(0);
	SET_VAO_ENTRY(0, vertex, position);

	glEnableVertexAttribArray(1);
	SET_VAO_ENTRY(1, vertex, color);

	DO_ERROR_CHECK();
}

void bulletDebugDrawer::drawLine(const btVector3& from,
                                 const btVector3& to,
                                 const btVector3& color)
{
	lines.push_back({
		{from.x(), from.y(), from.z()},
		{color.x(), color.y(), color.z()}
	});

	lines.push_back({
		{to.x(), to.y(), to.z()},
		{color.x(), color.y(), color.z()}
	});
}

void bulletDebugDrawer::flushLines(glm::mat4 cam) {
	if (!currentMode) {
		return;
	}

	databuf->buffer(lines.data(), lines.size() * sizeof(vertex));

	setDefaultGlFlags();
	bindVao(vao);
	shader->bind();
	shader->set("cam", cam);
	disable(GL_DEPTH_TEST);

	glPointSize(3.0f);
	glLineWidth(1.0f);
	glDrawArrays(GL_POINTS, 0, lines.size());
	glDrawArrays(GL_LINES, 0, lines.size());

	clearLines();
}

#endif
