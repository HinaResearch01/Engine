#pragma once

#include <string>
#include <stdexcept>
#include <format>
#include <wrl.h>
#include <d3d12.h>

struct Mdl { std::string name; };
struct Tex { std::string name; };
struct Snd { std::string name; };

namespace Tsumi {

/* 各種リソースの共通インターフェース */
class ResourceAPI {

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
	root, name;
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
	name;
	return false;
}

inline HRESULT ResourceAPI::SceneReset()
{
	return S_OK;
}


}
