#include <grend/animation.hpp>
#include <math.h>
#include <cstdio>

using namespace grendx;

size_t animation::findKeyframe(float delta) {
	assert(frametimes.size() != 0);
	assert(delta >= 0);

	size_t lower = 0;
	size_t upper = frametimes.size() - 1;
	size_t mid = (upper + lower)/2;

	while (lower != upper) {
		if (delta <= frametimes[mid]
			&& (mid == 0 || delta > frametimes[mid-1]))
		{
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

	return mid;
}

std::tuple<size_t, size_t, float> animation::interpFrames(float delta) {
	// TODO: not assert()
	assert(frametimes.size() > 0);
	assert(delta >= 0);

	float adj = fmod(delta, frametimes.back());
	size_t idx = findKeyframe(adj);
	size_t kdx = (idx + 1) % frametimes.size();

	assert(idx < frametimes.size());
	float interp = 0.0;

	if (kdx) {
		float range = frametimes[kdx] - frametimes[idx];
		interp = (adj - frametimes[idx])/range;

	} else if (frametimes[0] > 0) {
		interp = adj / frametimes[0];

	} else {
		interp = 1.0;
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
