#include <grend/camera.hpp>
#include <grend/glm-includes.hpp>

using namespace grendx;

void camera::set_direction(glm::vec3 dir) {
	direction = glm::normalize(dir);
	right = glm::normalize(glm::cross(glm::vec3(0, 1, 0), direction));
	up    = glm::normalize(glm::cross(direction, right));
}

glm::mat4 camera::projectionTransform(unsigned screen_x, unsigned screen_y) {
	float fov_y = (field_of_view_x * screen_y) / screen_x;
	return glm::perspective(
		glm::radians(fov_y),
		(1.f*screen_x) / screen_y, 0.1f, 100.0f);
}

glm::mat4 camera::viewTransform(void) {
	return glm::lookAt(position,
	                   position + direction,
	                   up);
}

glm::mat4 camera::viewProjTransform(unsigned screen_x, unsigned screen_y) {
	return projectionTransform(screen_x, screen_y) * viewTransform();
}
