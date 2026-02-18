#include "KamataCity.h"
#include "Framework/Component/Material/MaterialComponent.h"
#include "Framework/Component/Render/RenderComponent.h"

using namespace Tsumi::Framework;

void KamataCity::Init()
{
	IActor::name_ = "KamataCity";
	IActor::AddComp<MaterialComponent>();
	auto* rendComp = IActor::AddComp<RenderComponent>();
	rendComp->mesh = "kamata";
}
