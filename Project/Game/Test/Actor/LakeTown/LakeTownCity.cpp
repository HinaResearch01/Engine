#include "LakeTownCity.h"
#include "Framework/Component/Material/MaterialComponent.h"
#include "Framework/Component/Render/RenderComponent.h"

using namespace Tsumi::Framework;

void LakeTownCity::Init()
{
	IActor::name_ = "LakeTownCity";
	IActor::AddComp<MaterialComponent>();
	auto* rendComp = IActor::AddComp<RenderComponent>();
	rendComp->mesh = "laketown";
}
