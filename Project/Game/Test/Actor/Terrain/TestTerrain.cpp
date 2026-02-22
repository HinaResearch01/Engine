#include "TestTerrain.h"
#include "Framework/Component/Material/MaterialComponent.h"
#include "Framework/Component/Render/RenderComponent.h"

void TestTerrain::Init()
{
	IActor::name_ = "TestTerrain";
	IActor::AddComp<Tsumi::Framework::MaterialComponent>();
	auto* rendComp = IActor::AddComp<Tsumi::Framework::RenderComponent>();
	rendComp->mesh = "TestTerrain";
}
