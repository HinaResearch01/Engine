#pragma once

#include <string>
#include <stdexcept>
#include <format>
#include <wrl.h>
#include <d3d12.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <filesystem>

#include "Utils/Logger/UtilsLog.h"
#include "Utils/Func/UtilFunc.h"
#include  "Loader/Tex/TextureLoader.h"
#include "Loader/Mesh/MeshLoader.h"
#include "Loader/Model/ModelLoader.h"

struct mdl { std::string name; };
struct tex { std::string name; };
struct snd { std::string name; };

namespace tme {

/* 各種リソースの共通インターフェース */
class ResourceAPI {

public:
	/// <summary>
	/// 読み込み処理
	/// </summary>
	template <typename T>
	static HRESULT Load(const std::string& fullPath, const std::string& alias);

	/// <summary>
	/// 所持確認
	/// </summary>
//	template <typename T>
//	static bool Has(const std::string& name);

	/// <summary>
	/// シーンリセット処理
	/// </summary>
	static HRESULT SceneReset();
};


template <>
inline HRESULT ResourceAPI::Load<mdl>(const std::string& fullPath, const std::string& alias)
{
	HRESULT hr = Tsumi::Loader::ModelLoader::Load(fullPath, alias);

	if (FAILED(hr)) {
		Tsumi::Utils::Error(std::format(
			L"[ResourceAPI::Load<Model>] Failed to load model '{}' (HRESULT = 0x{:08X})",
			Tsumi::Utils::Utf8ToWstring(fullPath),
			static_cast<unsigned>(hr)
		));
		return hr;
	}

	return S_OK;
}

template <>
inline HRESULT ResourceAPI::Load<tex>(const std::string& fullPath, const std::string& alias)
{
	HRESULT hr = Tsumi::Loader::TextureLoader::Load(fullPath, alias);

	if (FAILED(hr)) {
		Tsumi::Utils::Error(std::format(
			L"[ResourceAPI::Load<Tex>] Failed to load texture '{}' (HRESULT = 0x{:08X})",
			Tsumi::Utils::Utf8ToWstring(fullPath),
			static_cast<unsigned>(hr)
		));
		return hr;
	}

	return S_OK;
}

inline HRESULT ResourceAPI::SceneReset()
{
	return S_OK;
}

}
