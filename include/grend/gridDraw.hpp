#pragma once

#include <grend/glManager.hpp>
#include <grend/glmIncludes.hpp>

namespace grendx {

class gridDrawer {
	public:
		// array of start, end, and color, each 3 floats
		struct vertex {
			glm::vec3 position;
			glm::vec3 color;
		};

		std::vector<vertex> lines;
		Buffer::ptr databuf;
		Vao::ptr vao;
		Program::ptr shader;

		glm::vec3 lastPosition;
		glm::vec3 lastExtent;
		glm::vec3 lastSpacing;

		gridDrawer();
		~gridDrawer();

		void buildGrid(const glm::vec3& position,
		               const glm::vec3& spacing = glm::vec3(1.f),
		               const glm::vec3& extent  = glm::vec3(100.f));

		void drawLine(const glm::vec3& from,
		              const glm::vec3& to,
		              const glm::vec3& color);

		void clearLines() { lines.clear(); };

		// XXX: flushLines will need a view/world transform, nop overload
		void flushLines() { /* lines.clear(); */ };
		void flushLines(glm::mat4 cam);
};

// namespace grendx
}
