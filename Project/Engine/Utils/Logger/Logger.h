#pragma once

#if defined(_WIN32)
#include <Windows.h>
#endif

#include <string_view>
#include <format>
#include <chrono>
#include <ctime>

namespace Tsumi::Utils {

class Logger {
public:
	enum class Level {
		Info,
		Warn,
		Error
	};

	template<typename... Args>
	static void Info(std::string_view fmt, Args&&... args) {
		Log(Level::Info, fmt, std::forward<Args>(args)...);
	}

	template<typename... Args>
	static void Warn(std::string_view fmt, Args&&... args) {
		Log(Level::Warn, fmt, std::forward<Args>(args)...);
	}

	template<typename... Args>
	static void Error(std::string_view fmt, Args&&... args) {
		Log(Level::Error, fmt, std::forward<Args>(args)...);
	}

private:
	template<typename... Args>
	static void Log(Level level, std::string_view fmt, Args&&... args);
};



template<typename... Args>
void Logger::Log(Level level, std::string_view fmt, Args&&... args)
{
#if defined(_DEBUG)
	std::string message =
		std::vformat(fmt, std::make_format_args(args...));

	const char* levelStr =
		(level == Level::Info) ? "[INFO] " :
		(level == Level::Warn) ? "[WARN] " :
		"[ERROR]";

	std::string final =
		std::string("[Tsumi] ") + levelStr + message + "\n";

#if defined(_WIN32)
	OutputDebugStringA(final.c_str());
#endif
#else
	(void)level;
	(void)fmt;
	((void)args, ...);
#endif
}

// 明示的インスタンス化（リンクエラー防止）
template void Logger::Log<>(Level, std::string_view);


}