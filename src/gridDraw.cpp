#include <grend-config.h>

#include <grend/renderContext.hpp>
#include <grend/gridDraw.hpp>

using namespace grendx;

gridDrawer::~gridDrawer() {};

// XXX: should probably be in glManager.hpp
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

gridDrawer::gridDrawer() {
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

void gridDrawer::buildGrid(const glm::vec3& position,
                           const glm::vec3& spacing,
                           const glm::vec3& extent)
{
	clearLines();

	glm::vec3 pos = {position.x, 0, position.z};
	align(pos.x, spacing.x);
	align(pos.z, spacing.z);

	glm::vec3 color = {1, 1, 1};

	for (float x = -extent.x; x < extent.x; x += spacing.x) {
		drawLine({x, 0, -extent.z}, {x, 0, extent.z}, color);
	}

	for (float z = -extent.z; z < extent.z; z += spacing.z) {
		drawLine({-extent.x, 0, z}, {extent.x, 0, z}, color);
	}
}

void gridDrawer::drawLine(const glm::vec3& from,
                          const glm::vec3& to,
                          const glm::vec3& color)
{
	lines.push_back({ from, color });
	lines.push_back({ to,   color });
}

void gridDrawer::flushLines(glm::mat4 cam) {
	databuf->buffer(lines.data(), lines.size() * sizeof(vertex));

	setDefaultGlFlags();
	bindVao(vao);
	shader->bind();
	shader->set("cam", cam);
	glDepthMask(GL_TRUE);
	enable(GL_DEPTH_TEST);

#if GLSL_VERSION == 330 || GLSL_VERSION == 430
	glPointSize(3.0f);
#endif
	glLineWidth(1.0f);
	glDrawArrays(GL_POINTS, 0, lines.size());
	glDrawArrays(GL_LINES, 0, lines.size());

	//clearLines();
	glDepthMask(GL_TRUE);
}
