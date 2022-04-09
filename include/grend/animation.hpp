#pragma once
#include <grend/glmIncludes.hpp>
#include <grend/TRS.hpp>

#include <string>
#include <map>
#include <unordered_map>
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

		virtual ~animation();

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

		void applyTransform(struct TRS& thing, float delta, float endtime);
		std::vector<animation::ptr> animations;
};


// maps node names -> animation channels
// sceneNode nodes store a corresponding hash, this way
// animations and models/skins can be loaded from seperate files
class animationMap
	: public std::unordered_map<uint32_t, std::vector<animationChannel::ptr>>
{
	public:
		typedef std::shared_ptr<animationMap> ptr;
		typedef std::weak_ptr<animationMap>   weakptr;

		float endtime = 0.0;
};

class animationCollection
	: public std::unordered_map<std::string, animationMap::ptr>
{
	public:
		typedef std::shared_ptr<animationCollection> ptr;
		typedef std::weak_ptr<animationCollection>   weakptr;
};

class animationTranslation : public animation {
	public:
		typedef std::shared_ptr<animationTranslation> ptr;
		typedef std::weak_ptr<animationTranslation>   weakptr;

		virtual ~animationTranslation();
		virtual void applyTransform(struct TRS& thing, float delta, float end);
		std::vector<glm::vec3> translations;
};

class animationRotation : public animation {
	public:
		typedef std::shared_ptr<animationRotation> ptr;
		typedef std::weak_ptr<animationRotation>   weakptr;

		virtual ~animationRotation();
		virtual void applyTransform(struct TRS& thing, float delta, float end);
		std::vector<glm::quat> rotations;
};

class animationScale : public animation {
	public:
		typedef std::shared_ptr<animationScale> ptr;
		typedef std::weak_ptr<animationScale>   weakptr;

		virtual ~animationScale();
		virtual void applyTransform(struct TRS& thing, float delta, float end);
		std::vector<glm::vec3> scales;
};

// namespace grendx;
}
