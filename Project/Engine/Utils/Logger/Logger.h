#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <memory>
#include <mutex>
#include <sstream>
#include <type_traits>

#include "ILogSink.h"

namespace Tsumi::Utils {

class Logger {
public:
	enum class Level {
		Info,
		Warn,
		Error
	};

	static void AddSink(std::unique_ptr<ILogSink> sink);

	template<typename... Args>
	static void Info(std::string_view msg, Args&&... args) {
		Log(Level::Info, msg, std::forward<Args>(args)...);
	}

	template<typename... Args>
	static void Warn(std::string_view msg, Args&&... args) {
		Log(Level::Warn, msg, std::forward<Args>(args)...);
	}

	template<typename... Args>
	static void Error(std::string_view msg, Args&&... args) {
		Log(Level::Error, msg, std::forward<Args>(args)...);
	}

private:
	static std::string ToLevelString(Level level);

	// wchar → UTF-8
	static std::string ToString(const std::wstring& ws);
	static std::string ToString(const wchar_t* ws);

	template<size_t N>
	static std::string ToString(const wchar_t(&ws)[N]) {
		return ToString(std::wstring(ws));
	}

	template<typename T>
	static void Append(std::ostringstream& oss, T&& v) {
		using U = std::remove_cvref_t<T>;

		if constexpr (
			std::is_same_v<U, std::wstring> ||
			std::is_same_v<U, wchar_t*> ||
			std::is_same_v<U, const wchar_t*> ||
			(std::is_array_v<U> && std::is_same_v<std::remove_extent_t<U>, wchar_t>)
			) {
			oss << ToString(v);
		}
		else {
			oss << std::forward<T>(v);
		}
	}

	template<typename... Args>
	static void Log(Level level, std::string_view msg, Args&&... args)
	{
#if defined(_DEBUG)
		std::ostringstream oss;

		oss << "[Tsumi] " << ToLevelString(level) << " ";
		oss << msg;
		((oss << ' ', Append(oss, std::forward<Args>(args))), ...);
		oss << '\n';

		std::lock_guard<std::mutex> lock(mutex_);
		for (auto& sink : sinks_) {
			sink->Write(oss.str());
		}
#else
		(void)level;
		(void)msg;
		((void)args, ...);
#endif
	}

private:
	static inline std::vector<std::unique_ptr<ILogSink>> sinks_;
	static inline std::mutex mutex_;
};

}
