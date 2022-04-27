#include <grend/camera.hpp>
#include <grend/glmIncludes.hpp>
#include <grend/boundingBox.hpp>
#include <grend/utility.hpp>
#include <grend/interpolation.hpp>

using namespace grendx;

void camera::slide(glm::vec3 target, float divisor, float delta) {
	position_ = interp::average(target, position_, divisor, delta);
}

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

void camera::setNear(float near) {
	near_ = near;
	updated = true;
}

void camera::setFar(float far) {
	far_ = far;
	updated = true;
}

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

// functions similarly to gl_FragCoord, but returning [0, 1] as screen coordinates
glm::vec4 camera::worldToScreenPosition(glm::vec3 pos) {
	// TODO: cache view/projection transforms, maybe
	glm::vec4 clip = viewProjTransform() * glm::vec4(pos, 1.0);

	clip.x /= clip.w;
	clip.y /= clip.w;
	clip.z /= clip.w;
	clip.w  = 1.0 / clip.w;

	clip.x = clip.x*0.5 + 0.5;
	clip.y = clip.y*0.5 + 0.5;

	return clip;
}

bool camera::onScreen(glm::vec4 pos) {
	return pos.x >= 0.f && pos.x <= 1.f
	    && pos.y >= 0.f && pos.y <= 1.f
	    && pos.z >= -1 && pos.z <= 1;
}

bool camera::sphereInFrustum(const BSphere& sphere) {
	recalculatePlanes();

	for (unsigned i = 0; i < 6; i++) {
		auto& p = planes[i];
		//float dist = p.n.x*v.x + p.n.y*v.y + p.n.z*v.z + p.d + r;
		float dist = glm::dot(p.n, sphere.center) + p.d + sphere.extent;

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
	float distances[6][8];

	// precalculate distances to avoid calculating them again in edge testing
	for (unsigned i = 0; i < 8; i++) {
		for (unsigned k = 0; k < 6; k++) {
			auto& p = planes[k];
			distances[k][i] = glm::dot(p.n, box.points[i]) + p.d;
		}
	}

	for (unsigned i = 0; i < 8 && !anyIn; i++) {
		bool pointInPlanes = true;

		for (unsigned k = 0; k < 6; k++) {
			if (distances[k][i] <= 0) {
				bool edgeIn = false;
				unsigned aPlane = k ^ 1;

				// Count the point as in if an adjacent edge is in the plane,
				// so that edges that pass through parallel (camera) planes will
				// be counted
				for (unsigned n = 0; n < 3; n++) {
					unsigned aPoint = i ^ (1 << n); 
					bool m = distances[k][aPoint] >= 0 && distances[aPlane][i] >= 0;
					edgeIn |= m;
				}

				// can't be in the plane if the point isn't in and no edges are in
				if (!edgeIn) {
					pointInPlanes = false;
					break;
				}
			}
		}

		anyIn |= pointInPlanes;
	}

	return anyIn;
}
