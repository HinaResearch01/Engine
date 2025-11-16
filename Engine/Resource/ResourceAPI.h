#pragma once

#include <string>
#include <stdexcept>
#include <format>
#include <wrl.h>
#include <d3d12.h>

#include "Utils/Logger/UtilsLog.h"
#include "Utils/Func/UtilFunc.h"
#include "Loader/Tex/TextureLoader.h"
#include "Resource/Tex/TextureManager.h"

struct Mdl { std::string name; };
struct Tex { std::string name; };
struct Snd { std::string name; };

namespace Tsumi {

/* 各種リソースの共通インターフェース */
class ResourceAPI {

private: // シングルトン
	ResourceAPI() = default;
	~ResourceAPI() = default;
	ResourceAPI(const ResourceAPI&) = delete;
	const ResourceAPI& operator=(const ResourceAPI&) = delete;

public:
	/// <summary>
	/// 読み込み処理
	/// </summary>
	template <typename T>
	HRESULT Load(const std::string& root, const std::string& name);

	/// <summary>
	/// 所持確認
	/// </summary>
	template <typename T>
	bool Has(const std::string& name);

	/// <summary>
	/// シーンリセット処理
	/// </summary>
	HRESULT SceneReset();
};


template <>
inline HRESULT ResourceAPI::Load<Mdl>(const std::string& root, const std::string& name)
{
	root,name;
	return S_OK;
}

template <>
inline HRESULT ResourceAPI::Load<Tex>(const std::string& root, const std::string& name)
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
inline bool ResourceAPI::Has<Mdl>(const std::string& name)
{
	name;
	return false;
}

template<>
inline bool ResourceAPI::Has<Tex>(const std::string& name)
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
