#include "UtilFunc.h"

namespace fs = std::filesystem;

std::wstring Tsumi::Utils::Func::Utf8ToWstring(const std::string& utf8)
{
    if (utf8.empty()) return L"";

    int len = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), static_cast<int>(utf8.size()), nullptr, 0);
    std::wstring wstr(len, 0);
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), static_cast<int>(utf8.size()), &wstr[0], len);
    return wstr;
}

std::string Tsumi::Utils::Func::WstringToUtf8(const std::wstring& wstr)
{
    if (wstr.empty()) return "";

    int len = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), static_cast<int>(wstr.size()), nullptr, 0, nullptr, nullptr);
    std::string str(len, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), static_cast<int>(wstr.size()), &str[0], len, nullptr, nullptr);
    return str;
}

std::string Tsumi::Utils::Func::MakeKeyFromRoot(
	const std::string& root,
	const std::string& name)
{
	fs::path p(name);
	fs::path full;

	// 絶対パスの場合
	if (p.is_absolute()) {
		if (!root.empty()) {
			try {
				fs::path rootp(root);
				fs::path rel = fs::relative(p, rootp);
				if (!rel.empty()) {
					return (rootp / rel).lexically_normal().string();
				}
			}
			catch (...) {
				// 相対化失敗時は絶対パスをそのまま使用
			}
		}
		return p.lexically_normal().string();
	}
	// 相対パスの場合
	else {
		if (!root.empty()) full = fs::path(root) / p;
		else full = p;
		return full.lexically_normal().string();
	}
}