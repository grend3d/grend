#include <grend/animation.hpp>
#include <math.h>
#include <cstdio>

using namespace grendx;

size_t animation::findKeyframe(float delta) {
	assert(frametimes.size() != 0);
	assert(delta >= 0);
	size_t idx = 0;

	for (size_t div = frametimes.size()/2; div; div /= 2) {
		if (delta <= frametimes[div]
			&& (idx == 0 || delta > frametimes[idx-1]))
		{
			return idx;
		}

		// TODO: need to set frametimes[0] to 0 when loading,
		//       just to make sure it's 0
		if (delta < frametimes[idx]) {
			assert(idx != 0);
			idx -= div;
		} else {
			idx += div;
		}
	}

	return idx;
}

std::tuple<size_t, size_t, float> animation::interpFrames(float delta) {
	// TODO: not assert()
	assert(frametimes.size() > 0);
	assert(delta >= 0);

	float adj = fmod(delta, frametimes.back());
	size_t idx = findKeyframe(adj);
	size_t kdx = (idx + 1) % frametimes.size();

	assert(idx < frametimes.size());
	/*
	if (idx == frametimes.size() - 1) {
		puts("asdf");
	}
	*/
	// XXX: TODO: proper time
	//float range = fabs(frametimes[kdx] - frametimes[idx]);
	//float interp = (adj - frametimes[idx])/range;
	float interp = 0.0;

	//assert(kdx);

	if (kdx) {
		float range = frametimes[kdx] - frametimes[idx];
		interp = (adj - frametimes[idx])/range;

	} else if (frametimes[0] > 0) {
		//float range = frametimes[kdx] - frametimes[idx];
		//interp = (adj - frametimes[idx])/range;
		//interp = 0.0;
		//float range = frametimes[0];
		interp = adj / frametimes[0];

	} else {
		interp = 0.0;
	}

	return {idx, kdx, interp};
}

void animationTranslation::applyTransform(struct TRS& thing, float delta) {
	auto [idx, kdx, interp] = interpFrames(delta);
	thing.position += glm::mix(translations[idx], translations[kdx], interp);
}

void animationRotation::applyTransform(struct TRS& thing, float delta) {
	auto [idx, kdx, interp] = interpFrames(delta);
	thing.rotation = glm::mix(rotations[idx], rotations[kdx], interp);
}

void animationScale::applyTransform(struct TRS& thing, float delta) {
	auto [idx, kdx, interp] = interpFrames(delta);
	thing.scale = glm::mix(scales[idx], scales[kdx], interp);
}
