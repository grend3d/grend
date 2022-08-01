#include <grend/logger.hpp>
#include <grend/sdlContext.hpp>
#include <list>

using namespace grendx;

static std::list<LogCallbackFunc> callbacks;

static void dispatch(LogType type, const std::string& message) {
	SDL_Log("%s", message.c_str());

	for (auto& f : callbacks) {
		f(type, message);
	}
}

void grendx::LogInfo(const std::string& message) {
	dispatch(LogType::Info, message);
}

void grendx::LogWarn(const std::string& message) {
	dispatch(LogType::Warning, message);
}

void grendx::LogError(const std::string& message) {
	dispatch(LogType::Error, message);
}

void grendx::LogCallback(LogCallbackFunc callback) {
	callbacks.push_back(callback);
}
