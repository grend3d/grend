#pragma once
#include <grend/glm-includes.hpp>
#include <grend/TRS.hpp>

#include <memory>
#include <vector>
#include <tuple>
#include <stddef.h>

namespace grendx {

// TODO: move to utilities
template <class T>
class spointers {
	public:
};

class animation {
	public:
		typedef std::shared_ptr<animation> ptr;
		typedef std::weak_ptr<animation>   weakptr;

		virtual void applyTransform(struct TRS& thing, float delta) = 0;
		size_t findKeyframe(float delta);
		std::vector<float> frametimes;
		// TODO: handle different interpolation methods
		// first, second frame, interpolation value
		std::tuple<size_t, size_t, float> interpFrames(float delta);
};

class animationTranslation : public animation {
	public:
		typedef std::shared_ptr<animationTranslation> ptr;
		typedef std::weak_ptr<animationTranslation>   weakptr;

		virtual void applyTransform(struct TRS& thing, float delta);
		std::vector<glm::vec3> translations;
};

class animationRotation : public animation {
	public:
		typedef std::shared_ptr<animationRotation> ptr;
		typedef std::weak_ptr<animationRotation>   weakptr;

		virtual void applyTransform(struct TRS& thing, float delta);
		std::vector<glm::quat> rotations;
};

class animationScale : public animation {
	public:
		typedef std::shared_ptr<animationScale> ptr;
		typedef std::weak_ptr<animationScale>   weakptr;

		virtual void applyTransform(struct TRS& thing, float delta);
		std::vector<glm::vec3> scales;
};

// namespace grendx;
}
