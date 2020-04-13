#include <grend/timers.hpp>
#include <chrono>

using namespace grendx;

void sma_counter::start(void) {
	begin = std::chrono::high_resolution_clock::now();
}

void sma_counter::stop(void) {
	auto end = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> secs = end - begin;

	add_sample(1.0 / secs.count());
}

double sma_counter::average(void) {
	double sum = 0;

	for (unsigned i = 0; i < samples.size(); i++) {
		sum += get_sample(i);
	}

	return sum / samples.size();
}

void sma_counter::add_sample(double sample) {
	samples[++frameptr % samples.size()] = sample;
}

double sma_counter::get_sample(int i) {
	return samples[(frameptr + i) % samples.size()];
}

#if 0
// old implemention
#define SMA_BUFSIZE 8
#define TARGET_FPS 60.0
static uint32_t frametimes[SMA_BUFSIZE];
static uint32_t frameptr = 0;

static double fps_sma(uint32_t t) {
	frametimes[frameptr] = t;
	frameptr = (frameptr + 1) % SMA_BUFSIZE;

	uint32_t sum = 0;
	for (unsigned i = 0; i < SMA_BUFSIZE; i++) {
		sum += frametimes[(frameptr + i) % SMA_BUFSIZE];
	}

	return 1.f/(sum / (float)SMA_BUFSIZE / 1000.f);
}

static double fps_sma(void) {
	uint32_t sum = 0;
	for (unsigned i = 0; i < SMA_BUFSIZE; i++) {
		sum += frametimes[i];
	}

	return 1.f/(sum / (float)SMA_BUFSIZE / 1000.f);
}

static std::pair<uint32_t, uint32_t> framems_minmax(void) {
	uint32_t min = 0xffffffff, max = 0;

	for (unsigned i = 0; i < SMA_BUFSIZE; i++) {
		min = (frametimes[i] < min)? frametimes[i] : min;
		max = (frametimes[i] > max)? frametimes[i] : max;
	}

	return {min, max};
}
#endif
