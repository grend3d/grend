#pragma once
#include <grend/glmIncludes.hpp>
#include <memory>

namespace grendx {

class camera {
	public:
		typedef std::shared_ptr<camera> ptr;
		typedef std::weak_ptr<camera> weakptr;

		enum projection {
			Perspective,
			Orthographic,
		};

		camera() {
			setFovx(100.0);
		}

		const glm::vec3& position() { return position_; };
		const glm::vec3& direction() { return direction_; };
		const glm::vec3& velocity() { return velocity_; };
		const glm::vec3& right() { return right_; };
		const glm::vec3& up() { return up_; };
		const enum projection project() { return projection_; };
		const float fovx() { return fovx_; };
		const float fovy() { return fovy_; };
		const float scale() { return scale_; };
		const float near() { return near_; };
		const float far() { return far_; };

		void setPosition(glm::vec3 pos);
		void updatePosition(float delta); // increment by velocity
		void setDirection(glm::vec3 dir);
		void setDirection(glm::vec3 dir, glm::vec3 up);
		void setVelocity(glm::vec3 vel);
		void setViewport(unsigned x, unsigned y);
		void setProjection(enum projection proj);
		void setFovx(float angle);
		void setFovy(float angle);
		void setScale(float scale);
		void setNear(float near);
		void setFar(float far);
		void recalculatePlanes(void);

		glm::mat4 projectionTransform(void);
		glm::mat4 viewTransform(void);
		glm::mat4 viewProjTransform(void);
		glm::vec4 worldToScreenPosition(glm::vec3 pos);
		bool onScreen(glm::vec4 pos);

		bool sphereInFrustum(const glm::vec3& pos, float r);
		bool boxInFrustum(const struct AABB& box);
		bool boxInFrustum(const struct OBB& box);

	private:
		glm::vec3 position_ = glm::vec3(0);
		glm::vec3 direction_ = glm::vec3(0, 0, 1);
		glm::vec3 velocity_ = glm::vec3(0, 0, 0);

		glm::vec3 right_ = glm::vec3(1, 0, 0);
		glm::vec3 up_ = glm::vec3(0, 1, 0);
		float fovx_ = 100.0;
		float fovy_ = 0.0;
		float near_ = 0.1;
		float far_ = 100.0;
		float scale_ = 1.0; // orthographic scale
		unsigned screenX = 1, screenY = 1;

		// track whether there have been any changes since the last plane
		// generation, frustum testing functions check this to determine
		// whether to generate some fresh planes
		bool updated = true;

		// TODO: maybe have plane class?
		struct plane {
			glm::vec3 n;
			float d;
		} planes[6];

		enum {
			pLeft,
			pRight,
			pBottom,
			pTop,
			pNear,
			pFar,
		};

		enum projection projection_ = projection::Perspective;
};

// namespace grendx
}
