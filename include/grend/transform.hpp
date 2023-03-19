#pragma once

#include <grend/glmIncludes.hpp>
#include <grend/TRS.hpp>

namespace grendx {

class transformState {
	private:
		// TODO: transform setters/getters, cache in entity
		TRS transform;
		TRS origTransform;

		bool updated = true;
		bool isDefault = true;
		glm::mat4 cachedTransformMatrix;

	public:
		const TRS& getTRS();
		const TRS& getOrig();
		glm::mat4 getMatrix();
		bool hasDefaultTransform(void) const { return isDefault; }

		void set(const TRS& t);
		void setPosition(const glm::vec3& position);
		void setScale(const glm::vec3& scale);
		void setRotation(const glm::quat& rotation);
		// TODO: methods for working with euler angles

		// cache for renderQueue state, queueCache only changed externally
		// from renderQueue::add(), queueUpdated changed both from
		// setTransfrom() and renderQueue::add()
		// TODO: documented, link up documentation
		// TODO: old piece of code, is this still needed?
		struct {
			bool      updated = true;
			glm::mat4 transform;
			glm::vec3 center;
		} queueCache;
};

// namespace grendx;
};
