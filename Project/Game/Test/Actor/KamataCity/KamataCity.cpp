#include "KamataCity.h"
#include "Framework/Component/Material/MaterialComponent.h"
#include "Framework/Component/Render/RenderComponent.h"

void KamataCity::Init()
{
	IActor::name_ = "KamataCity";
	IActor::AddComp<Tsumi::Framework::MaterialComponent>();
	auto* rendComp = IActor::AddComp<Tsumi::Framework::RenderComponent>();
	rendComp->mesh = "kamata";
}
