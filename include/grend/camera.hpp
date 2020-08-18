#pragma once
#include <grend/glm-includes.hpp>
#include <memory>

namespace grendx {

class camera {
	public:
		typedef std::shared_ptr<camera> ptr;
		typedef std::weak_ptr<camera> weakptr;

		glm::vec3 position = glm::vec3(0);
		glm::vec3 velocity = glm::vec3(0);
		glm::vec3 direction = glm::vec3(0, 0, 1);

		glm::vec3 right = glm::vec3(1, 0, 0);
		glm::vec3 up = glm::vec3(0, 1, 0);
		float field_of_view_x = 100.0;

		void set_direction(glm::vec3 dir);
		glm::mat4 projectionTransform(unsigned screenx, unsigned screeny);
		glm::mat4 viewTransform(void);
		glm::mat4 viewProjTransform(unsigned screenx, unsigned screeny);
};

// namespace grendx
}
