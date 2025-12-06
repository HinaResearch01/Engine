#pragma once

#include <string>
#include <stdexcept>
#include <format>
#include <wrl.h>
#include <d3d12.h>

#include "Utils/Logger/UtilsLog.h"
#include "Utils/Func/UtilFunc.h"
#include "Resource/Tex/TextureManager.h"
#include  "Loader/Tex/TextureLoader.h"

struct mdl { std::string name; };
struct tex { std::string name; };
struct snd { std::string name; };

namespace Tsumi {

/* 各種リソースの共通インターフェース */
class ResourceAPI {

public:
	/// <summary>
	/// 読み込み処理
	/// </summary>
	template <typename T>
	static HRESULT Load(const std::string& root, const std::string& name);

	/// <summary>
	/// 所持確認
	/// </summary>
	template <typename T>
	static bool Has(const std::string& name);

	/// <summary>
	/// シーンリセット処理
	/// </summary>
	static HRESULT SceneReset();
};


template <>
inline HRESULT ResourceAPI::Load<mdl>(const std::string& root, const std::string& name)
{
	root,name;
	return S_OK;
}

template <>
inline HRESULT ResourceAPI::Load<tex>(const std::string& root, const std::string& name)
{
	HRESULT hr = Loader::TextureLoader::LoadFromFile(root, name);

	if (FAILED(hr)) {
		Utils::Error(std::format(
			L"[ResourceAPI::Load<Tex>] Failed to load texture '{}{}' (HRESULT = 0x{:08X})",
			Utils::Utf8ToWstring(root),
			Utils::Utf8ToWstring(name),
			static_cast<unsigned>(hr)
		));
		return hr;
	}

	return S_OK;
}

template<>
inline bool ResourceAPI::Has<mdl>(const std::string& name)
{
	name;
	return false;
}

template<>
inline bool ResourceAPI::Has<tex>(const std::string& name)
{
	bool exists = Resource::TextureManager::GetInstance()->Has(name);

	if (!exists) {
		Utils::Error(std::format(
			L"[ResourceAPI::Has<Tex>] Texture '{}' not found or not loaded.",
			Utils::Utf8ToWstring(name)
		));
		return false;
	}

	return true;
}

inline HRESULT ResourceAPI::SceneReset()
{
	return S_OK;
}

}
