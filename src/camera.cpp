#include <grend/camera.hpp>
#include <grend/glm-includes.hpp>
#include <grend/boundingBox.hpp>

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

void camera::setProjection(enum projection proj) {
	projection_ = proj;
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

void camera::setScale(float scale) {
	scale_ = scale;
	updated = true;
}

// for debugging output
// TODO: remove
#include <stdio.h>

void camera::recalculatePlanes(void) {
	// no updates, current planes are still good
	if (!updated) return;

	auto p = projectionTransform();
	auto v = viewTransform();
	// planes in world space
	auto m = p*v;

	planes[pLeft].n.x = m[0][3] + m[0][0];
	planes[pLeft].n.y = m[1][3] + m[1][0];
	planes[pLeft].n.z = m[2][3] + m[2][0];
	planes[pLeft].d   = m[3][3] + m[3][0];

	planes[pRight].n.x = m[0][3] - m[0][0];
	planes[pRight].n.y = m[1][3] - m[1][0];
	planes[pRight].n.z = m[2][3] - m[2][0];
	planes[pRight].d   = m[3][3] - m[3][0];

	planes[pBottom].n.x = m[0][3] + m[0][1];
	planes[pBottom].n.y = m[1][3] + m[1][1];
	planes[pBottom].n.z = m[2][3] + m[2][1];
	planes[pBottom].d   = m[3][3] + m[3][1];

	planes[pTop].n.x = m[0][3] - m[0][1];
	planes[pTop].n.y = m[1][3] - m[1][1];
	planes[pTop].n.z = m[2][3] - m[2][1];
	planes[pTop].d   = m[3][3] - m[3][1];

	planes[pNear].n.x = m[0][3] + m[0][2];
	planes[pNear].n.y = m[1][3] + m[1][2];
	planes[pNear].n.z = m[2][3] + m[2][2];
	planes[pNear].d   = m[3][3] + m[3][2];

	planes[pFar].n.x = m[0][3] - m[0][2];
	planes[pFar].n.y = m[1][3] - m[1][2];
	planes[pFar].n.z = m[2][3] - m[2][2];
	planes[pFar].d   = m[3][3] - m[3][2];

	for (unsigned i = 0; i < 6; i++) {
		// normalize planes
		float nn = glm::length(planes[i].n);
		planes[i].n /= nn;
		planes[i].d /= nn;
	}

	updated = false;
}

glm::mat4 camera::projectionTransform(void) {
	switch (projection_) {
		case projection::Orthographic:
			return glm::ortho(0.f, scale_*screenX, 0.f, scale_*screenY, near_, far_);

		case projection::Perspective:
		default:
			return glm::perspective(
				glm::radians(fovy_),
				(1.f*screenX) / screenY, near_, far_);
	}
}

glm::mat4 camera::viewTransform(void) {
	return glm::lookAt(position_,
	                   position_ + direction_,
	                   up_);
}

glm::mat4 camera::viewProjTransform(void) {
	return projectionTransform() * viewTransform();
}

bool camera::sphereInFrustum(const glm::vec3& v, float r) {
	recalculatePlanes();

	for (unsigned i = 0; i < 6; i++) {
		auto& p = planes[i];
		//float dist = p.n.x*v.x + p.n.y*v.y + p.n.z*v.z + p.d + r;
		float dist = glm::dot(p.n, v) + p.d + r;

		if (dist < 0) {
			return false;
		}
	}

	return true;
}

bool camera::boxInFrustum(const struct AABB& box) {
	recalculatePlanes();

	for (unsigned i = 0; i < 6; i++) {
		auto& p = planes[i];
		auto dist = [&] (const glm::vec3& v) {
			//return p.n.x*v.x + p.n.y*v.y + p.n.z*v.z + p.d;
			return glm::dot(p.n, v) + p.d;
		};

		if (dist(box.min) < 0 && dist(box.max) < 0) {
			return false;
		}
	}

	return true;
}

bool camera::boxInFrustum(const struct OBB& box) {
	recalculatePlanes();
	bool anyIn = false;

	// XXX: if the center is in the frustum, it's in the frustum.
	//      hack to get around things popping out of existence when
	//      the object is in frustum but the vertices are outside
	// TODO: proper test for this, can't quite figure out a quick way
	//       and don't want to spend too much time on it...
	bool centerInPlanes = true;
	for (unsigned k = 0; k < 6; k++) {
		auto& p = planes[k];
		float dist = glm::dot(p.n, box.center) + p.d;

		if (dist < 0) {
			centerInPlanes = false;
			break;
		}
	}

	anyIn = centerInPlanes;

	for (unsigned i = 0; i < 8 && !anyIn; i++) {
		bool pointInPlanes = true;

		for (unsigned k = 0; k < 6; k++) {
			auto& p = planes[k];
			float dist = glm::dot(p.n, box.points[i]) + p.d;

			if (dist < 0) {
				pointInPlanes = false;
				break;
			}
		}

		anyIn = anyIn || pointInPlanes;
	}

	return anyIn;
}
