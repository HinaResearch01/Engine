#include "LakeTownCity.h"
#include "Framework/Component/Material/MaterialComponent.h"
#include "Framework/Component/Render/RenderComponent.h"

void LakeTownCity::Init()
{
	IActor::name_ = "LakeTown";
	IActor::AddComp<Tsumi::Framework::MaterialComponent>();
	auto* rendComp = IActor::AddComp<Tsumi::Framework::RenderComponent>();
	rendComp->mesh = "laketown";
}
