#include <grend/transform.hpp>

namespace grendx {

const TRS& transformState::getTRS() {
	return transform;
};

const TRS& transformState::getOrig() {
	return origTransform;
};

glm::mat4 transformState::getMatrix() {
	if (updated) {
		cachedTransformMatrix = transform.getTransform();
		updated = false;
		// note that queueCache.updated isn't changed here
	}

	return cachedTransformMatrix;
};

void transformState::set(const TRS& t) {
	if (isDefault) {
		origTransform = t;
	}

	updated = true;
	queueCache.updated = true;
	isDefault = false;
	transform = t;
}

void transformState::setPosition(const glm::vec3& position) {
	TRS temp = getTRS();
	temp.position = position;
	set(temp);
}

void transformState::setScale(const glm::vec3& scale) {
	TRS temp = getTRS();
	temp.scale = scale;
	set(temp);
}

void transformState::setRotation(const glm::quat& rotation) {
	TRS temp = getTRS();
	temp.rotation = rotation;
	set(temp);
}

// namespace grendx
}
