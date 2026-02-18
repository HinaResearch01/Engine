#include "TestSpotLight.h"
#include "TestSpotLightComp.h"

void TestSpotLight::Init()
{
	IActor::name_ = "TestSpotLight";
	IActor::AddComp<TestSpotLightComp>();

	auto* tr = IActor::GetComponent<Tsumi::Framework::TransformComponent>();
	// 上から下を照らす
	tr->srt.translate= { 0.0f, 10.0f, 0.0f };
	tr->srt.rotate = { -90.0f, 0.0f, 0.0f };
}
