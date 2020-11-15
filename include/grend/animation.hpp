#pragma once
#include <grend/glmIncludes.hpp>
#include <grend/TRS.hpp>

#include <string>
#include <map>
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

class animationGroup;
class animationCollection;

class animation {
	public:
		typedef std::shared_ptr<animation> ptr;
		typedef std::weak_ptr<animation>   weakptr;

		virtual void applyTransform(struct TRS& thing, float delta, float end) = 0;
		size_t findKeyframe(float delta);
		// TODO: handle different interpolation methods
		// first, second frame, interpolation value
		std::tuple<size_t, size_t, float> interpFrames(float delta, float end);
		std::vector<float> frametimes;
};

class animationChannel {
	public:
		typedef std::shared_ptr<animationChannel> ptr;
		typedef std::weak_ptr<animationChannel>   weakptr;

		void applyTransform(struct TRS& thing, float delta);
		std::vector<animation::ptr> animations;
		std::shared_ptr<animationGroup> group;
};

void applyChannelVecTransforms(std::vector<animationChannel::ptr>& channels,
                               struct TRS& thing,
                               float delta);

class animationGroup {
	public:
		typedef std::shared_ptr<animationGroup> ptr;
		typedef std::weak_ptr<animationGroup>   weakptr;

		std::shared_ptr<animationCollection> collection;
		std::string name;

		float endtime = 0.0;
		float weight = 1.0;
};

class animationCollection {
	public:
		typedef std::shared_ptr<animationCollection> ptr;
		typedef std::weak_ptr<animationCollection>   weakptr;

		void applyTransform(struct TRS& thing, float delta);
		std::map<std::string, animationGroup::weakptr> groups;
};

class animationTranslation : public animation {
	public:
		typedef std::shared_ptr<animationTranslation> ptr;
		typedef std::weak_ptr<animationTranslation>   weakptr;

		virtual void applyTransform(struct TRS& thing, float delta, float end);
		std::vector<glm::vec3> translations;
};

class animationRotation : public animation {
	public:
		typedef std::shared_ptr<animationRotation> ptr;
		typedef std::weak_ptr<animationRotation>   weakptr;

		virtual void applyTransform(struct TRS& thing, float delta, float end);
		std::vector<glm::quat> rotations;
};

class animationScale : public animation {
	public:
		typedef std::shared_ptr<animationScale> ptr;
		typedef std::weak_ptr<animationScale>   weakptr;

		virtual void applyTransform(struct TRS& thing, float delta, float end);
		std::vector<glm::vec3> scales;
};

// namespace grendx;
}
