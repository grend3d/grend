#include <grend/jobQueue.hpp>

using namespace grendx;

jobQueue::jobQueue(unsigned concurrency) {
	for (unsigned i = 0; i < concurrency; i++) {
		workers.push_back(std::thread(&jobQueue::worker, this));
	}
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
	return addAsync(std::packaged_task<bool()>(job));
}

std::future<bool> jobQueue::addDeferred(std::function<bool()> job) {
	return addDeferred(std::packaged_task<bool()>(job));
}

std::future<bool> jobQueue::addAsync(std::packaged_task<bool()> job) {
	std::lock_guard<std::mutex> g(mtx);
	auto fut = job.get_future();
	asyncJobs.push_back(std::move(job));
	waiters.notify_one(); // wake up a thread, if any are available
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

void jobQueue::worker(void) {
	// TODO: loop condition
	while (running) {
		auto job = getAsync();
		job();
	}

	std::terminate();
}

#include <iostream>
std::packaged_task<bool()> jobQueue::getAsync(void) {
	std::unique_lock<std::mutex> slock(mtx);

	if (asyncJobs.empty()) {
		std::cerr
			<< "got here, thread "
			<< std::this_thread::get_id()
			<< " waiting for a job"
			<< std::endl;

		waiters.wait(slock, [this]{ return !asyncJobs.empty(); });
	}

	std::cerr
		<< "(empty: " << asyncJobs.empty() << ") "
		<< "got here, thread "
		<< std::this_thread::get_id() << std::endl;

	auto job = std::move(asyncJobs.front());
	asyncJobs.pop_front();
	return job;
}
