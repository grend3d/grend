#include <grend/animation.hpp>
#include <grend/utility.hpp>
#include <math.h>
#include <cstdio>

using namespace grendx;

//#define DUMP_ANIMATION_INFO

// non-pure virtual destructors for rtti
animation::~animation() {};
animationTranslation::~animationTranslation() {};
animationRotation::~animationRotation() {};
animationScale::~animationScale() {};

#ifdef DUMP_ANIMATION_INFO
#define FERR(...) fprintf(stderr, __VA_ARGS__)
#else
#define FERR(...) /* nop */
#endif

size_t animation::findKeyframe(float delta) {
	assert(frametimes.size() != 0);
	assert(delta >= 0);

	/*
	for (size_t n = 0; n < frametimes.size(); n++) {
		if (delta <= frametimes[n]) {
			return n;
		}
	}

	return frametimes.size() - 1;
	*/

	size_t lower = 0;
	size_t upper = frametimes.size() - 1;
	size_t mid = (upper + lower)/2;

	while (lower != upper) {
		float dp = delta - (1.f/60);
		FERR("frame: mid: %lu, lower: %lu, upper: %lu\n",
		     mid, lower, upper);

		if (dp <= frametimes[mid] && (mid == 0 || dp > frametimes[mid-1])) {
			FERR("frame %lu (%p)\n", mid, this);
			return mid;
		}

		if (delta < frametimes[mid]) {
			upper = mid - 1;
			mid = (upper + lower)/2;

		} else {
			lower = mid + 1;
			mid = (upper + lower)/2;
		}
	}

	FERR("frame %lu (%p) (f)\n", mid, this);
	return mid;
}

std::tuple<size_t, size_t, float>
animation::interpFrames(float delta, float end) {
	// TODO: not assert()
	assert(frametimes.size() > 0);
	assert(delta >= 0);

	float adj = fmod(delta, end);
	size_t frame = findKeyframe(adj);
	size_t idx = (frame - 1) % frametimes.size();
	size_t kdx = frame;

	assert(idx < frametimes.size());
	float interp = 0.0;

	if (kdx) {
		float range = frametimes[kdx] - frametimes[idx];
		interp = (frametimes[kdx] - adj)/range;
	}

	else if (frametimes[0] == 0 && idx > 0) {
		float range = frametimes[idx] - frametimes[idx-1];
		interp = (end - adj)/range;
	}

	FERR("d: %f, end time: %f, interp: %f, idx: %lu, kdx: %lu, "
	     "fidx: %f, fkdx: %f\n",
		adj, end, interp, idx, kdx, frametimes[idx], frametimes[kdx]);

	return {idx, kdx, interp};
}

void animationChannel::applyTransform(struct TRS& thing,
                                      float delta,
                                      float endtime)
{
	//assert(group != nullptr);
	struct TRS asdf = thing;

	for (auto& ptr : animations) {
		FERR("anim: delta: %f, endtime %f\n", delta, endtime);
		ptr->applyTransform(asdf, delta, endtime);
		// TODO: reimplement animation weighting, could take weight as a parameter
		//asdf = mixtrs(asdf, foo, group->weight);
	}

	// or could return the resulting transform
	thing = asdf;
}

void animationTranslation::applyTransform(struct TRS& thing, float delta, float end) {
	auto [idx, kdx, interp] = interpFrames(delta, end);
	thing.position = glm::mix(translations[kdx], translations[idx], interp);
}

void animationRotation::applyTransform(struct TRS& thing, float delta, float end) {
	auto [idx, kdx, interp] = interpFrames(delta, end);
	thing.rotation = glm::mix(rotations[kdx], rotations[idx], interp);
}

void animationScale::applyTransform(struct TRS& thing, float delta, float end) {
	auto [idx, kdx, interp] = interpFrames(delta, end);
	thing.scale = glm::mix(scales[kdx], scales[idx], interp);
}
