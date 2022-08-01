#pragma once

#include <string>
#include <functional>

#include <fmt/core.h>

namespace grendx {

enum LogType {
	Info,
	Warning,
	Error,
};

void LogInfo(const std::string& message);
void LogWarn(const std::string& message);
void LogError(const std::string& message);

#define LogFmt(...)      LogInfo(fmt::format(__VA_ARGS__));
#define LogInfoFmt(...)  LogInfo(fmt::format(__VA_ARGS__));
#define LogWarnFmt(...)  LogWarn(fmt::format(__VA_ARGS__));
#define LogErrorFmt(...) LogError(fmt::format(__VA_ARGS__));

using LogCallbackFunc = std::function<void(enum LogType, const std::string&)>;
void LogCallback(LogCallbackFunc callback);

// namespace grendx
}
