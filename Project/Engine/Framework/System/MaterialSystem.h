#pragma once

#include <vector>
#include <unordered_map>
#include <memory>
#include <string>

namespace Tsumi::Framework {

using TextureHandle = std::string;
using MaterialHandle = std::string;

struct MaterialDesc
{
	TextureHandle albedo = "";
	TextureHandle normal = "";
	float metallic = 0.0f;
	float roughness = 1.0f;

	bool operator==(const MaterialDesc& rhs) const
	{
		return albedo == rhs.albedo &&
			normal == rhs.normal &&
			metallic == rhs.metallic &&
			roughness == rhs.roughness;
	}
};


/*  */
class MaterialSystem {

public:
	/// <summary>
	/// コンストラクタ
	/// </summary>
	MaterialSystem() = default;

	/// <summary>
	/// デストラクタ
	/// </summary>
	~MaterialSystem() = default;

	static MaterialSystem& Get();

	MaterialHandle CreateMaterial(const MaterialDesc& desc);

	const MaterialDesc& GetMaterialDesc(MaterialHandle h) const;

private:

	size_t Hash(const MaterialDesc& desc) const;

public:
	std::vector<MaterialDesc> materials_;
	std::unordered_map<size_t, MaterialHandle> lookup_;

};

}
