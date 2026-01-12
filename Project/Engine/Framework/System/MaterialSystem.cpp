#include "MaterialSystem.h"

using namespace Tsumi::Framework;

MaterialSystem& Tsumi::Framework::MaterialSystem::Get()
{
	// TODO: return ステートメントをここに挿入します
}

MaterialHandle Tsumi::Framework::MaterialSystem::CreateMaterial(const MaterialDesc& desc)
{
	return MaterialHandle();
}

const MaterialDesc& Tsumi::Framework::MaterialSystem::GetMaterialDesc(MaterialHandle h) const
{
	// TODO: return ステートメントをここに挿入します
}

size_t Tsumi::Framework::MaterialSystem::Hash(const MaterialDesc& desc) const
{
	return size_t();
}
