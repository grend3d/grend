#pragma once

#include <grend/IoC.hpp>

#include <thread>
#include <future>
#include <vector>
#include <list>
#include <utility>
#include <memory>
#include <condition_variable>
#include <chrono>

namespace grendx {

class jobQueue : public IoC::Service {
	public:
		typedef std::shared_ptr<jobQueue> ptr;
		typedef std::weak_ptr<jobQueue>   weakptr;

		jobQueue(unsigned concurrency = 2*std::thread::hardware_concurrency());
		virtual ~jobQueue();

		std::future<bool> addAsync(std::function<bool()> job);
		std::future<bool> addDeferred(std::function<bool()> job);
		std::future<bool> addAsync(std::packaged_task<bool()> job);
		std::future<bool> addDeferred(std::packaged_task<bool()> job);

		// run the queued deferred jobs (should be called from the main thread)
		void runDeferred(void);
		bool runSingleDeferred(void);

	private:
		// worker main loop
		void worker(void);
		// blocks if there's no available jobs
		std::packaged_task<bool()> getAsync(void);

		bool running = true;
		std::vector<std::thread> workers;
		std::mutex mtx;
		std::condition_variable waiters;

		// jobs that can be run in their own thread
		std::list<std::packaged_task<bool()>> asyncJobs;
		// jobs that must run syncronously, on the main thread
		// (eg. anything that touches openGL)
		std::list<std::packaged_task<bool()>> deferredJobs;
};

template <typename T>
bool jobFinished(std::future<T>& f) {
	return f.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
}

// namespace grendx
}
