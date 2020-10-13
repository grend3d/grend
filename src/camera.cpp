#include <grend/camera.hpp>
#include <grend/glm-includes.hpp>

using namespace grendx;

void camera::setPosition(glm::vec3 pos) {
	position_ = pos;
	updated = true;
}

void camera::updatePosition(float delta) {
	position_ += velocity_.z * direction_ * delta;
	position_ += velocity_.y * up_        * delta;
	position_ += velocity_.x * right_     * delta;

	updated = true;
}

void camera::setDirection(glm::vec3 dir) {
	direction_ = glm::normalize(dir);
	right_     = glm::normalize(glm::cross(glm::vec3(0, 1, 0), direction_));
	up_        = glm::normalize(glm::cross(direction_, right_));
	updated = true;
}

void camera::setDirection(glm::vec3 dir, glm::vec3 upwards) {
	direction_ = glm::normalize(dir);
	up_        = upwards;
	right_     = glm::normalize(glm::cross(up_, dir));
	updated = true;
}

void camera::setVelocity(glm::vec3 vel) {
	velocity_ = vel;
}

void camera::setViewport(unsigned x, unsigned y) {
	screenX = x;
	screenY = y;
	setFovx(fovx_); // (slightly counterintuitively) recalculate fovy
	updated = true;
}

void camera::setFovx(float angle) {
	fovx_ = angle;
	fovy_ = (fovx_ * screenY) / screenX;
	updated = true;
}

void camera::setFovy(float angle) {
	fovy_ = angle;
	fovx_ = (fovy_ * screenX) / screenY;
	updated = true;
}

void camera::recalculatePlanes(void) {
	// no updates, current planes are still good
	if (!updated) return;

	auto p = projectionTransform();
	auto v = viewTransform();

	for (unsigned i = 0; i < 6; i++) {
		planes[i].a = 1.0;
	}

	updated = false;
}

glm::mat4 camera::projectionTransform(void) {
	//float fov_y = (field_of_view_x * screen_y) / screen_x;
	return glm::perspective(
		glm::radians(fovy_),
		(1.f*screenX) / screenY, near_, far_);
}

glm::mat4 camera::viewTransform(void) {
	return glm::lookAt(position_,
	                   position_ + direction_,
	                   up_);
}

glm::mat4 camera::viewProjTransform(void) {
	return projectionTransform() * viewTransform();
}

bool camera::sphereInFrustum(const glm::vec3& pos, float r) {
	// TODO: implement
	return true;
}

bool camera::aabbInFrustum(const struct AABB& box) {
	// TODO: implement
	return true;
}
