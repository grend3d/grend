#pragma once
#include <vector>
#include <chrono>
#include <memory>
#include <string>
#include <map>
#include <stdint.h>

namespace grendx {

class sma_counter {
	public:
		sma_counter(unsigned n_samples=8) {
			samples.resize(n_samples, 0);
		}

		void start(void);
		void stop(void);
		double average(void);
		double last(void);

	private:
		void add_sample(double sample);
		double get_sample(int i);

		std::vector<double> samples;
		size_t frameptr = 0;

		std::chrono::time_point<std::chrono::high_resolution_clock> begin;
		//uint32_t begin = 0;
};

namespace profile {

void newFrame(void);
void endFrame(void);
void startGroup(std::string name);
void endGroup(void);

typedef std::chrono::time_point<std::chrono::high_resolution_clock> timepoint;

struct timer {
	timepoint begin;
	timepoint end;
	bool active;

	double duration(void) {
		std::chrono::duration<double> secs = end - begin;
		return secs.count();
	}
};

struct group {
	std::map<std::string, timer> times;
	std::map<std::string, group> subgroups;

	timer groupTimer;
};

// shared pointer to avoid copying data when examining a particular
// frame capture, makes handling multiple frame captures easy
std::shared_ptr<group> getFrame(void);

// namespace profile
}

// namespace grendx
}
