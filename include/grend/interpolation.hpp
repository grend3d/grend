#pragma once

namespace grendx::interp {

template <typename T>
T average(T target, T current, float divisor, float delta) {
	float fdelta = (1.f / 60) / delta; // normalize to 60fps
	float fdiv = divisor*fdelta;

	return (target + (fdiv - 1.f)*current) / fdiv;
}

template <typename T>
T linear(T target, T current, float factor, float delta) {
	float fdelta = (1.f / 60) / delta; // normalize to 60fps
	return current + (target - current) / (factor * fdelta);
}

// TODO: smoothstep, bicubic, etc

};
