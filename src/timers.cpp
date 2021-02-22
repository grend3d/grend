#include <grend/timers.hpp>
#include <grend/sdlContext.hpp>
#include <chrono>

using namespace grendx;

void sma_counter::start(void) {
	begin = std::chrono::high_resolution_clock::now();
	//begin = SDL_GetTicks();
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

double sma_counter::last(void) {
	return get_sample(0);
}

void sma_counter::add_sample(double sample) {
	samples[++frameptr % samples.size()] = sample;
}

double sma_counter::get_sample(int i) {
	return samples[(frameptr + i) % samples.size()];
}

using namespace grendx::profile;

static std::shared_ptr<group> frameGroup = nullptr;
static std::vector<group*> groupStack;

void grendx::profile::newFrame(void) {
	frameGroup = std::make_shared<group>();
	frameGroup->groupTimer.begin = std::chrono::high_resolution_clock::now();
	frameGroup->groupTimer.active = true;

	groupStack.clear();
	groupStack.push_back(frameGroup.get());
}

void grendx::profile::endFrame(void) {
	// TODO: error
	if (!frameGroup) return;

	frameGroup->groupTimer.end = std::chrono::high_resolution_clock::now();
	frameGroup->groupTimer.active = false;
}

std::shared_ptr<group> grendx::profile::getFrame(void) {
	return frameGroup;
}

void grendx::profile::startGroup(std::string name) {
	if (groupStack.empty()) return;

	(groupStack.back())->subgroups[name] = group();
	group& sub = (groupStack.back())->subgroups[name];

	sub.groupTimer.begin  = std::chrono::high_resolution_clock::now();
	sub.groupTimer.active = true;

	groupStack.push_back(&sub);
}

void grendx::profile::endGroup(void) {
	if (groupStack.empty()) return;

	group& cur = *groupStack.back();
	cur.groupTimer.end = std::chrono::high_resolution_clock::now();
	cur.groupTimer.active = false;

	groupStack.pop_back();
}
