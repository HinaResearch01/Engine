#include "Logger.h"

#if defined(_WIN32)
#include <Windows.h>
#endif

namespace Tsumi::Utils {

void Logger::AddSink(std::unique_ptr<ILogSink> sink)
{
	std::lock_guard<std::mutex> lock(mutex_);
	sinks_.emplace_back(std::move(sink));
}

std::string Logger::ToLevelString(Level level)
{
	switch (level) {
		case Level::Info:  return "[INFO]";
		case Level::Warn:  return "[WARN]";
		case Level::Error: return "[ERROR]";
		default:           return "[UNKNOWN]";
	}
}

std::string Logger::ToString(const std::wstring& ws)
{
#if defined(_WIN32)
	if (ws.empty()) return {};

	int size = WideCharToMultiByte(
		CP_UTF8, 0,
		ws.data(), static_cast<int>(ws.size()),
		nullptr, 0, nullptr, nullptr
	);

	std::string result(size, '\0');
	WideCharToMultiByte(
		CP_UTF8, 0,
		ws.data(), static_cast<int>(ws.size()),
		result.data(), size, nullptr, nullptr
	);
	return result;
#else
	return std::string(ws.begin(), ws.end());
#endif
}

std::string Logger::ToString(const wchar_t* ws)
{
	return ws ? ToString(std::wstring(ws)) : std::string{};
}

}
