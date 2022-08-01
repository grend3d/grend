#include <grend/jobQueue.hpp>
#include <grend/logger.hpp>

using namespace grendx;

jobQueue::jobQueue(unsigned concurrency) {
#ifdef __EMSCRIPTEN__
	// TOOO: some way to do background tasks on webgl, it's JS after all

#else
	for (unsigned i = 0; i < concurrency; i++) {
		workers.push_back(std::thread(&jobQueue::worker, this));
	}
#endif
	//workers.push_back(std::thread(&jobQueue::worker, this));
	//workers.push_back(std::thread(&jobQueue::worker, this));
}

jobQueue::~jobQueue() {
	/*
	for (auto& thr : workers) {
		// detached threads should be stopped once jobQueue is deleted, I think
		thr.detach();
	}
	*/
}

std::future<bool> jobQueue::addAsync(std::function<bool()> job) {
#ifdef __EMSCRIPTEN__
	return addDeferred(std::packaged_task<bool()>(job));
#else
	return addAsync(std::packaged_task<bool()>(job));
#endif
}

std::future<bool> jobQueue::addDeferred(std::function<bool()> job) {
	return addDeferred(std::packaged_task<bool()>(job));
}

std::future<bool> jobQueue::addAsync(std::packaged_task<bool()> job) {
	std::lock_guard<std::mutex> g(mtx);
	auto fut = job.get_future();
#ifdef __EMSCRIPTEN__
	deferredJobs.push_back(std::move(job));
#else
	asyncJobs.push_back(std::move(job));
	waiters.notify_one(); // wake up a thread, if any are available
#endif
	return fut;
}

std::future<bool> jobQueue::addDeferred(std::packaged_task<bool()> job) {
	std::lock_guard<std::mutex> g(mtx);
	auto fut = job.get_future();
	deferredJobs.push_back(std::move(job));
	return fut;
}

void jobQueue::runDeferred(void) {
	std::lock_guard<std::mutex> g(mtx);

	for (auto& job : deferredJobs) {
		job();
	}

	deferredJobs.clear();
}

bool jobQueue::runSingleDeferred(void) {
	std::lock_guard<std::mutex> g(mtx);
	bool ran = false;

	if (!deferredJobs.empty()) {
		auto& job = deferredJobs.front();
		job();

		deferredJobs.pop_front();
		ran = true;
	}

	return ran;
}

void jobQueue::worker(void) {
	// TODO: loop condition
	while (running) {
		auto job = getAsync();
		job();
	}

	std::terminate();
}

#include <iostream>
#include <SDL.h>
std::packaged_task<bool()> jobQueue::getAsync(void) {
	std::unique_lock<std::mutex> slock(mtx);

	if (asyncJobs.empty()) {
		// TODO: debug statements hurt performance
		LogFmt("[job queue] got here, thread {} waiting for job",
		       std::hash<std::thread::id>{}(std::this_thread::get_id()));

		waiters.wait(slock, [this]{ return !asyncJobs.empty(); });
	}

	LogFmt("[job queue] (empty: {}) got here, thread {}",
		asyncJobs.empty(),
		std::hash<std::thread::id>{}(std::this_thread::get_id()));

	auto job = std::move(asyncJobs.front());
	asyncJobs.pop_front();
	return job;
}
