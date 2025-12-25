#pragma once

#include "Utils/Logger/UtilsLog.h"
#include "Utils/Func/UtilFunc.h"
#include  "Loader/Tex/TextureLoader.h"
#include "Loader/Mesh/MeshLoader.h"
#include "Loader/Model/ModelLoader.h"
#include "Resource/Tex/TextureManager.h"
#include "Resource/Mesh/MeshManager.h"

struct mdl {};
struct tex {};
struct snd {};

namespace tme::API {

/* 各種アセットの共通インターフェース */
class AssetLoader {

public:
	/// <summary>
	/// 読み込み処理
	/// </summary>
	template <typename T>
	static HRESULT Load(const std::string& fullPath, const std::string& alias);

	/// <summary>
	/// シーンリセット処理
	/// </summary>
	static HRESULT SceneReset();
};


template <>
inline HRESULT AssetLoader::Load<mdl>(const std::string& fullPath, const std::string& alias)
{
	HRESULT hr = Tsumi::Loader::ModelLoader::Load(fullPath, alias);

	if (FAILED(hr)) {
		Tsumi::Utils::Error(std::format(
			L"[AssetLoader::Load<Model>] Failed to load model '{}' (HRESULT = 0x{:08X})",
			Tsumi::Utils::Utf8ToWstring(fullPath),
			static_cast<unsigned>(hr)
		));
		return hr;
	}

	return S_OK;
}

template <>
inline HRESULT AssetLoader::Load<tex>(const std::string& fullPath, const std::string& alias)
{
	HRESULT hr = Tsumi::Loader::TextureLoader::Load(fullPath, alias);

	if (FAILED(hr)) {
		Tsumi::Utils::Error(std::format(
			L"[AssetLoader::Load<Tex>] Failed to load texture '{}' (HRESULT = 0x{:08X})",
			Tsumi::Utils::Utf8ToWstring(fullPath),
			static_cast<unsigned>(hr)
		));
		return hr;
	}

	return S_OK;
}

inline HRESULT AssetLoader::SceneReset()
{
	using namespace Tsumi::Resource;
	TextureManager::GetInstance()->UnloadAll();
	MeshManager::GetInstance()->UnloadAll();
	return S_OK;
}


}