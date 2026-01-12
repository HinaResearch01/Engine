#pragma once
#include <unordered_map>
#include <memory>
#include <string>

namespace Tsumi::Framework {

// 前方宣言
class MaterialInstance;
struct RenderMaterial;

/*  */
class MaterialSystem {

public:
	// コンストラクタ
	MaterialSystem() = default;

	// デストラクタ
	~MaterialSystem() = default;

	// 登録
	void RegisterMaterial(const std::string& name, RenderMaterial* mat);

	// 
	MaterialInstance* Acquire(const std::string& name);

private:
	MaterialInstance* CreateInstance(RenderMaterial* mat);

private:
	std::unordered_map<std::string, RenderMaterial*> materials_;
	std::unordered_map<std::string, std::unique_ptr<MaterialInstance>> instances_;
};

}
