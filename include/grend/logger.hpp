#pragma once

#include <string>
#include <functional>

#include <format>

namespace grendx {

enum LogType {
	Info,
	Warning,
	Error,
};

void LogInfo(const std::string& message);
void LogWarn(const std::string& message);
void LogError(const std::string& message);

#define LogFmt(...)      LogInfo(std::format(__VA_ARGS__));
#define LogInfoFmt(...)  LogInfo(std::format(__VA_ARGS__));
#define LogWarnFmt(...)  LogWarn(std::format(__VA_ARGS__));
#define LogErrorFmt(...) LogError(std::format(__VA_ARGS__));

using LogCallbackFunc = std::function<void(enum LogType, const std::string&)>;
void LogCallback(LogCallbackFunc callback);

// namespace grendx
}
