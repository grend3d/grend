#include <grend/plane.hpp>

using namespace grendx;

bool plane::inPlane(glm::vec3 pos, float radius) {
	return distance(pos) + radius >= 0;
}

float plane::distance(glm::vec3 pos) {
	return glm::dot(n, pos) + d;
}
