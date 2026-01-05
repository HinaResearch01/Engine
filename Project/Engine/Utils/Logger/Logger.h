#pragma once

#if defined(_WIN32)
#include <Windows.h>
#endif

#include <string>
#include <string_view>
#include <sstream>
#include <type_traits>

namespace Tsumi::Utils {

class Logger {
public:
	enum class Level {
		Info,
		Warn,
		Error
	};

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
	// ----------------------------
	// wchar → UTF-8
	// ----------------------------
	static std::string ToString(const std::wstring& ws)
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

	static std::string ToString(const wchar_t* ws)
	{
		return ws ? ToString(std::wstring(ws)) : std::string{};
	}

	template<size_t N>
	static std::string ToString(const wchar_t(&ws)[N])
	{
		return ToString(std::wstring(ws));
	}

	// ----------------------------
	// 引数をストリームに追加
	// ----------------------------
	template<typename T>
	static void Append(std::ostringstream& oss, T&& v)
	{
		using U = std::remove_cvref_t<T>;

		if constexpr (
			std::is_same_v<U, std::wstring> ||
			std::is_same_v<U, wchar_t*> ||
			std::is_same_v<U, const wchar_t*> ||
			(std::is_array_v<U> && std::is_same_v<std::remove_extent_t<U>, wchar_t>)
			)
		{
			oss << ToString(v);
		}
		else
		{
			oss << std::forward<T>(v);
		}
	}

	template<typename... Args>
	static void Log(Level level, std::string_view msg, Args&&... args)
	{
#if defined(_DEBUG)
		std::ostringstream oss;
		oss << msg;

		// ★ 正しいフォールド式
		((oss << ' ', Append(oss, std::forward<Args>(args))), ...);

		const char* levelStr =
			(level == Level::Info) ? "[INFO] " :
			(level == Level::Warn) ? "[WARN] " :
			"[ERROR] ";

		std::string output =
			std::string("[Tsumi] ") + levelStr + oss.str() + "\n";

#if defined(_WIN32)
		OutputDebugStringA(output.c_str());
#endif
#else
		(void)level;
		(void)msg;
		((void)args, ...);
#endif
	}
};

} // namespace Tsumi::Utils
