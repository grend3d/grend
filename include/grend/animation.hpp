#pragma once
#include <grend/glm-includes.hpp>
#include <grend/TRS.hpp>

#include <memory>
#include <vector>
#include <stddef.h>

namespace grendx {

// TODO: move to utilities
template <class T>
class spointers {
	public:
		typedef std::shared_ptr<T> ptr;
		typedef std::weak_ptr<T>   weakptr;
};

class animation : public spointers<animation> {
	public:
		virtual void applyTransform(struct TRS& thing, float delta) = 0;
		size_t findKeyframe(float delta);
		std::vector<float> frametimes;
};

class animationTranslation {
	public:
		virtual void applyTransform(struct TRS& thing, float delta);
		std::vector<std::vector<glm::vec3>> translations;
};

class animationRotation {
	public:
		virtual void applyTransform(struct TRS& thing, float delta);
		std::vector<std::vector<glm::quat>> rotations;
};

class animationScale {
	public:
		virtual void applyTransform(struct TRS& thing, float delta);
		std::vector<std::vector<glm::vec3>> scales;
};

// namespace grendx;
}
