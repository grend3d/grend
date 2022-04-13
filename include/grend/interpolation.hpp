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

// TODO: another version of this that does smoothstep
template <typename T>
class smoothed {
	T target;
	T current;
	// average over 16 frames (normalized at 60fps)
	float division = 16;

	public:
		smoothed() {};
		smoothed(const T& initial, float div = 16.f)
			: target(initial),
			  current(initial),
			  division(div)
			{};

		void update(float delta) {
			current = average(target, current, division, delta);
		}

		operator const T& () const { return current; };

		const T& get(void) const {
			return current;
		}

		void set(T& value) {
			target = value;
		}

		void reset() {
			target = current = T();
		}

		void reset(T& value) {
			target = current = value;
		}

		void operator = (const T& other) {
			target = other;
		}

		void operator += (const T& other) {
			target += other;
		}

		void operator -= (const T& other) {
			target -= other;
		}

		const T& operator*() const {
			return current;
		}
};

};
